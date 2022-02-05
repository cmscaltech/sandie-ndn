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
PipelineInterestsFixed::PipelineInterestsFixed(Face &face, size_t windowSize)
    : PipelineInterests(face), m_windowSize{windowSize} {
}

PipelineInterestsFixed::~PipelineInterestsFixed() {
    this->stop();
}

void PipelineInterestsFixed::process() {
    std::vector<PendingInterest> pendingInterests{};
    size_t size = 0, index = 0;

    auto clearPendingInterests = [&]() {
        pendingInterests.clear();
        index = 0;
    };

    auto getNextPendingInterests = [&]() {
        clearPendingInterests();

        size = std::min(static_cast<int>(m_windowSize - m_pit->size()), 64);
        pendingInterests.reserve(size);
        size = m_requestQueue.try_dequeue_bulk(pendingInterests.begin(), size);

        return size;
    };

    while (isValid()) {
        face->loop();
        onTimeout();

        if (m_pit->size() >= m_windowSize) {
            continue; // Wait for data packets
        }

        if (size == 0 && getNextPendingInterests() == 0) {
            continue;
        }

        std::vector<ndn::Block> pkts;
        std::transform(pendingInterests.begin() + index,
                       pendingInterests.begin() + index + size,
                       std::back_inserter(pkts),
                       [](PendingInterest pi) -> ndn::Block {
                           return pi.interestBlockValue;
                       });

        uint16_t n = 0;
        if (!face->send(std::move(pkts), size, &n)) {
            LOG_FATAL("unable to send Interest packets on face");

            stop();
            return;
        }

        for (n += index; index < n; ++index, --size) {
            pendingInterests[index].markAsExpressed();
            // Timeouts handler
            m_queue->push(pendingInterests[index].pitTokenValue);
            m_pit->emplace(pendingInterests[index].pitTokenValue,
                           pendingInterests[index]);
        }
    }
}

void PipelineInterestsFixed::onData(std::shared_ptr<ndn::Data> &&data,
                                    ndn::lp::PitToken &&pitToken) {
    auto pitKey = getPITTokenValue(std::move(pitToken));

    if (m_pit->find(pitKey) == m_pit->end()) {
        LOG_DEBUG("unexpected Data packet dropped");
        return;
    }

    m_pit->erase(pitKey);
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
        auto interest = getWireDecode(m_pit->at(pitKey).interestBlockValue);
        interest->refreshNonce();

        auto timeoutCounter = m_pit->at(pitKey).timeoutCounter;
        m_pit->erase(pitKey);

        if (!this->enqueueInterest(std::move(interest), timeoutCounter)) {
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
            m_queue->pop(); // Remove already satisfied entries
            continue;
        }

        auto pendingInterest = it->second;
        if (!pendingInterest.hasExpired()) {
            return;
        }

        m_pit->erase(it);
        m_queue->pop();

        auto interest = getWireDecode(pendingInterest.interestBlockValue);
        interest->refreshNonce();

        auto timeoutCounter = pendingInterest.timeoutCounter + 1;
        LOG_DEBUG("timeout (%li) for %s", timeoutCounter,
                  interest->getName().toUri().c_str());

        if (timeoutCounter < 8) {
            if (!this->enqueueInterest(std::move(interest), timeoutCounter)) {
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
