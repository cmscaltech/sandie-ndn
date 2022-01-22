/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Sichen Song <songsichen@cs.ucla.edu>
 *
 * MIT License
 *
 * Copyright (c) 2021 University of California, Los Angeles
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
#include "pipeline-interests-aimd.hpp"

namespace ndnc {
PipelineInterestsAimd::PipelineInterestsAimd(Face &face, size_t windowSize)
    : PipelineInterests(face), m_windowSize{windowSize}, m_windowIncCounter{0},
      m_lastDecrease{ndn::time::steady_clock::now()} {
}

PipelineInterestsAimd::~PipelineInterestsAimd() {
    this->stop();
}

void PipelineInterestsAimd::process() {
    std::vector<PendingInterest> batch;
    size_t batchSize = 0;
    size_t batchIndex = 0;

    auto clearBatch = [&]() {
        batch.clear();
        batchIndex = 0;
    };

    auto fillBatch = [&]() {
        clearBatch();
        batchSize =
            std::min(static_cast<int>(m_windowSize - m_pit->size()), 64);
        batch.reserve(batchSize);

        batchSize = m_requestQueue.try_dequeue_bulk(batch.begin(), batchSize);
        return batchSize;
    };

    while (this->isValid()) {
        this->face->loop();
        this->onTimeout();

        if (m_pit->size() >= m_windowSize) {
            continue; // Wait for data packets
        }

        if (batchSize == 0 && fillBatch() == 0) {
            continue;
        }

        std::vector<ndn::Block> interests;
        interests.reserve(batchSize);

        for (size_t i = batchIndex; i < batchIndex + batchSize; ++i) {
            interests.emplace_back(ndn::Block(batch[i].interest));
        }

        uint16_t n;
        if (!face->send(std::move(interests), batchSize, &n)) {
            LOG_FATAL("unable to send Interest packets on face");

            this->stop();
            return;
        }

        for (n += batchIndex; batchIndex < n; ++batchIndex, --batchSize) {
            batch[batchIndex].markAsExpressed();
            m_queue->push(batch[batchIndex].pitToken); // to handle timeouts
            m_pit->emplace(batch[batchIndex].pitToken, batch[batchIndex]);
        }
    }
}

void PipelineInterestsAimd::onData(std::shared_ptr<ndn::Data> &&data,
                                   ndn::lp::PitToken &&pitToken) {
    auto pitKey = getPITTokenValue(std::move(pitToken));

    if (m_pit->find(pitKey) == m_pit->end()) {
        LOG_DEBUG("unexpected Data packet dropped");
        return;
    }

    if (data->getCongestionMark()) {
        decreaseWindow();
        LOG_DEBUG("ECN received");
    }

    m_pit->erase(pitKey);
    enqueueData(std::move(data)); // Enqueue Data into Response queue

    increaseWindow();
}

void PipelineInterestsAimd::onNack(std::shared_ptr<ndn::lp::Nack> &&nack,
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

void PipelineInterestsAimd::onTimeout() {
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

        LOG_DEBUG("timeout (%li) for %s", timeoutCnt,
                  interest->getName().toUri().c_str());

        decreaseWindow();

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

void PipelineInterestsAimd::decreaseWindow() {
    ndn::time::steady_clock::time_point now = ndn::time::steady_clock::now();
    if (now - m_lastDecrease < MAX_RTT) {
        return;
    }

    LOG_DEBUG("window decrease at %li", m_windowSize);
    m_windowSize /= 2;
    m_windowIncCounter = 0;
    m_windowSize = std::max(m_windowSize, MIN_WINDOW);
    m_lastDecrease = now;
}

void PipelineInterestsAimd::increaseWindow() {
    m_windowIncCounter++;
    if (m_windowIncCounter >= m_windowSize) {
        m_windowSize++;
        m_windowIncCounter = 0;
        m_windowSize = std::min(m_windowSize, MAX_WINDOW);
    }
}
}; // namespace ndnc
