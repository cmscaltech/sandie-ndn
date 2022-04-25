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
PipelineInterestsAimd::PipelineInterestsAimd(face::Face &face,
                                             size_t windowSize)
    : PipelineInterests(face), m_ssthresh{windowSize}, m_windowSize{64},
      m_windowIncCounter{0}, m_lastDecrease{ndn::time::steady_clock::now()} {
}

PipelineInterestsAimd::~PipelineInterestsAimd() {
    this->stop();
}

void PipelineInterestsAimd::process() {
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

        auto n = face->send(std::move(pkts), size);
        if (n < 0) {
            LOG_FATAL("unable to send Interest packets on face");

            stop();
            return;
        }

        m_counters->nTxPackets += n;

        for (n += index; index < (size_t)n; ++index, --size) {
            pendingInterests[index].markAsExpressed();
            // Timeouts handler
            m_queue->push(pendingInterests[index].pitTokenValue);
            m_pit->emplace(pendingInterests[index].pitTokenValue,
                           pendingInterests[index]);
        }
    }
}

void PipelineInterestsAimd::onData(std::shared_ptr<ndn::Data> &&data,
                                   ndn::lp::PitToken &&pitToken) {
    ++m_counters->nRxPackets;

    auto pitKey = getPITTokenValue(std::move(pitToken));

    auto it = m_pit->find(pitKey);
    if (it == m_pit->end()) {
        LOG_DEBUG("unexpected Data packet dropped");
        ++m_counters->nUnexpectedRxPackets;
        return;
    }

    if (data->getCongestionMark()) {
        decreaseWindow();
        LOG_DEBUG("ECN received");
    }

    m_counters->delay += it->second.timeSinceExpressed();

    m_pit->erase(it);
    enqueueData(std::move(data)); // Enqueue Data into Response queue

    increaseWindow();
}

void PipelineInterestsAimd::onNack(std::shared_ptr<ndn::lp::Nack> &&nack,
                                   ndn::lp::PitToken &&pitToken) {
    ++m_counters->nNacks;

    auto pitKey = getPITTokenValue(std::move(pitToken));

    if (m_pit->find(pitKey) == m_pit->end()) {
        LOG_DEBUG("unexpected NACK for packet dropped");
        ++m_counters->nUnexpectedRxPackets;
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

void PipelineInterestsAimd::onTimeout() {
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

        ++m_counters->nTimeouts;

        m_pit->erase(it);
        m_queue->pop();

        auto interest = getWireDecode(pendingInterest.interestBlockValue);
        interest->refreshNonce();

        auto timeoutCounter = pendingInterest.timeoutCounter + 1;
        LOG_DEBUG("timeout (%li) for %s", timeoutCounter,
                  interest->getName().toUri().c_str());

        decreaseWindow();

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

void PipelineInterestsAimd::decreaseWindow() {
    auto now = ndn::time::steady_clock::now();
    if (now - m_lastDecrease < MAX_RTT) {
        return;
    }

    LOG_DEBUG("window decrease at %li", m_windowSize);

    m_windowSize /= 2;
    m_windowIncCounter = 0;
    m_windowSize = std::max(m_windowSize, MIN_WINDOW);
    m_ssthresh = m_windowSize;
    m_lastDecrease = now;
}

void PipelineInterestsAimd::increaseWindow() {
    // slow start
    if (m_windowSize < m_ssthresh) {
        m_windowSize += 1;
        return;
    }
    // congestion avoidance
    m_windowIncCounter++;
    if (m_windowIncCounter >= m_windowSize) {
        m_windowSize++;
        m_windowIncCounter = 0;
        m_windowSize = std::min(m_windowSize, MAX_WINDOW);
    }
}
}; // namespace ndnc
