/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2021 California Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "logger/logger.hpp"
#include "pipeline-interests-fixed.hpp"

namespace ndnc {
PipelineInterestsFixed::PipelineInterestsFixed(Face &face, size_t maxWindowSize)
    : PipelineInterests(face), m_maxWindowSize{maxWindowSize} {}

PipelineInterestsFixed::~PipelineInterestsFixed() {
    this->stop();
}

void PipelineInterestsFixed::process() {
    std::vector<PendingInterest> batch;
    size_t size = 0;
    size_t index = 0;

    auto clearBatch = [&]() {
        batch.clear();
        index = 0;
    };

    auto fillBatch = [&]() {
        clearBatch();
        size = std::min(static_cast<int>(m_maxWindowSize - m_pit->size()), 64);
        batch.reserve(size);

        size = m_requestQueue.try_dequeue_bulk(batch.begin(), size);
        return size;
    };

    while (this->isValid()) {
        this->face->loop();
        this->onTimeout();

        if (m_pit->size() >= m_maxWindowSize) {
            continue; // Wait for data packets
        }

        if (size == 0 && fillBatch() == 0) {
            continue;
        }

        std::vector<ndn::Block> interests;
        interests.reserve(size);

        for (size_t i = index; i < index + size; ++i) {
            interests.emplace_back(ndn::Block(batch[i].interest));
        }

        uint16_t n;
        if (!face->send(std::move(interests), size, &n)) {
            LOG_FATAL("unable to send Interest packets on face");

            this->stop();
            return;
        }

        for (n += index; index < n; ++index, --size) {
            batch[index].markAsExpressed();
            m_queue->push(batch[index].pitEntry); // to handle timeouts
            m_pit->emplace(batch[index].pitEntry, batch[index]);
        }
    }
}

void PipelineInterestsFixed::onData(std::shared_ptr<ndn::Data> &&data,
                                    ndn::lp::PitToken &&pitToken) {
    auto pitToken = getPITTokenValue(std::move(pitToken));

    if (m_pit->find(pitToken) == m_pit->end()) {
        LOG_DEBUG("unexpected Data packet dropped");
        return;
    }

    m_pit->erase(pitToken);
    enqueueData(std::move(data)); // Enqueue Data into Response queue
}

void PipelineInterestsFixed::onNack(std::shared_ptr<ndn::lp::Nack> &&nack,
                                    ndn::lp::PitToken &&pitToken) {

    auto pitKey = getPITTokenValue(std::move(pitToken));
    if (m_pit->find(pitKey) == m_pit->end()) {
        LOG_DEBUG("unexpected NACK for packet dropped");
        return;
    }

    if (nack->getReason() == ndn::lp::NackReason::NONE) {
        return;
    }

    LOG_DEBUG(
        "received NACK with reason=%i",
        static_cast<typename std::underlying_type<ndn::lp::NackReason>::type>(
            nack->getReason()));

    switch (nack->getReason()) {
    case ndn::lp::NackReason::DUPLICATE: {
        auto interest = getWireDecode(m_pit->at(pitKey).interest);
        auto timeoutCnt = m_pit->at(pitKey).timeoutCnt;
        m_pit->erase(pitKey);

        interest->refreshNonce();

        if (!this->enqueueInterest(std::move(interest), timeoutCnt)) {
            LOG_FATAL("unable to enqueue Interest for duplicate NACK");
            enqueueData(nullptr); // Enqueue null to mark error
            return;
        }
        break;
    }
    default:
        LOG_FATAL("received unsupported NACK packet");
        enqueueData(nullptr); // Enqueue null to mark error
        m_pit->erase(pitKey);
        break;
    }
}

void PipelineInterestsFixed::onTimeout() {
    while (!m_queue->empty()) {
        auto it = m_pit->find(m_queue->front());

        if (it == m_pit->end()) {
            // Remove already satisfied entries
            m_queue->pop();
            continue;
        }

        auto pitValue = it->second;
        if (!pitValue.isExpired()) {
            return;
        }

        m_pit->erase(it);
        m_queue->pop();

        auto interest = getWireDecode(pitValue.interest);
        auto timeoutCnt = pitValue.timeoutCnt + 1;
        interest->refreshNonce();

        LOG_DEBUG("timeout (%li): %s", timeoutCnt,
                  interest->getName().toUri().c_str());

        if (timeoutCnt < 8) {
            if (!this->enqueueInterest(std::move(interest), timeoutCnt)) {
                LOG_FATAL("unable to enqueue Interest on timeout");
                enqueueData(nullptr); // Enqueue null to mark error
                return;
            }
        } else {
            LOG_FATAL("reached maximum number of timeout retries");
            enqueueData(nullptr); // Enqueue null to mark error
            return;
        }
    }
}
}; // namespace ndnc
