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
    this->close();
}

void PipelineInterestsAimd::open() {
    std::vector<PendingInterest> pendingInterests{};
    int size = 0, index = 0;

    auto getNextPendingInterests = [&]() {
        index = 0;
        size = popPendingInterests(
            pendingInterests,
            std::min(static_cast<int>(m_windowSize - m_pit->size()), 64));
        return size;
    };

    while (!isClosed()) {
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
                           return pi.getInterestBlockValue();
                       });

        auto n = face->send(&pkts, size);
        if (n < 0) {
            LOG_FATAL("unable to send Interest packets on face");

            close();
            return;
        }

        m_counters.tx += n;

        for (n += index; index < n; ++index, --size) {
            pendingInterests[index].markAsExpressed();
            // Timeout handler
            m_piq->push(pendingInterests[index].getPITTokenValue());
            m_pit->emplace(pendingInterests[index].getPITTokenValue(),
                           pendingInterests[index]);
        }
    }
}

void PipelineInterestsAimd::onData(std::shared_ptr<ndn::Data> &&data,
                                   ndn::lp::PitToken &&pitToken) {
    ++m_counters.rx;

    auto pitKey = getPITTokenValue(std::move(pitToken));
    auto it = m_pit->find(pitKey);

    if (it == m_pit->end()) {
        LOG_DEBUG("unexpected Data packet dropped");
        ++m_counters.rxUnexpected;
        return;
    }

    if (data->getCongestionMark()) {
        decreaseWindow();
        LOG_DEBUG("ECN received");
    }

    m_counters.delay += it->second.getTimeSinceExpressed();

    // Enqueue Data into Response queue
    if (!pushData(it->second.getConsumerId(), std::move(data))) {
        this->close();
        return;
    }
    m_pit->erase(it);

    increaseWindow();
}

void PipelineInterestsAimd::onNack(std::shared_ptr<ndn::lp::Nack> &&nack,
                                   ndn::lp::PitToken &&pitToken) {
    ++m_counters.nack;

    auto pitKey = getPITTokenValue(std::move(pitToken));
    auto it = m_pit->find(pitKey);

    if (it == m_pit->end()) {
        LOG_DEBUG("unexpected NACK for packet dropped");
        ++m_counters.rxUnexpected;
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
        if (!this->refreshPITEntry(pitKey)) {
            if (!pushData(it->second.getConsumerId(), nullptr)) {
                // Enqueue null to mark error
                this->close();
                return;
            }
            m_pit->erase(pitKey);
            return;
        }
        break;
    }
    default:
        LOG_FATAL("received unsupported NACK packet");

        if (!pushData(it->second.getConsumerId(), nullptr)) {
            // Enqueue null to mark error
            this->close();
            return;
        }
        m_pit->erase(pitKey);
        break;
    }
}

void PipelineInterestsAimd::onTimeout() {
    while (!m_piq->empty()) {
        auto it = m_pit->find(m_piq->front());

        if (it == m_pit->end()) {
            m_piq->pop(); // Remove already satisfied entries
            continue;
        }

        if (!it->second.isExpired()) {
            return;
        }

        ++m_counters.timeout;

        auto consumerId = it->second.getConsumerId();

        if (it->second.hasReachedMaximumNumOfRetries()) {
            LOG_FATAL("reached maximum number of timeout retries");

            if (!pushData(consumerId, nullptr)) {
                // Enqueue null to mark error
                this->close();
                return;
            }

            m_pit->erase(it);
            return;
        }

        auto pitKey = it->first;

        LOG_DEBUG("timeout (%li) for %s", it->second.getRetriesCount() + 1,
                  it->second.getInterest()->getName().toUri().c_str());

        decreaseWindow();

        if (!this->refreshPITEntry(pitKey, true)) {
            LOG_FATAL("unable to refresh Interest on timeout");

            if (!pushData(consumerId, nullptr)) {
                // Enqueue null to mark error
                this->close();
                return;
            }

            m_pit->erase(it);
            m_piq->pop();
            return;
        } else {
            m_piq->pop();
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
