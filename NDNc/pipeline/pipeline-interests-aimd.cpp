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
PipelineInterestsAimd::PipelineInterestsAimd(Face &face, size_t size)
    : PipelineInterests(face), m_windowSize{size},
      m_lastDecrease{ndn::time::steady_clock::now()} {}

PipelineInterestsAimd::~PipelineInterestsAimd() {
    this->end();
}

void PipelineInterestsAimd::process() {
    while (this->isValid()) {
        face->loop();
        onTimeout();

        if (m_pit->size() >= m_windowSize) {
            continue;
        }

        std::vector<PendingInterest> pendingInterests(m_windowSize -
                                                      m_pit->size());
        size_t n = m_requestQueue.try_dequeue_bulk(
            pendingInterests.begin(), m_windowSize - m_pit->size());

        if (n == 0) {
            continue;
        }

        std::vector<ndn::Block> interests;
        interests.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            auto key = pendingInterests[i].pitEntry;

            auto entry = m_pit->emplace(key, pendingInterests[i]).first;
            interests.emplace_back(std::move(pendingInterests[i].interest));

            m_queue->push(key);
            entry->second.markAsExpressed();
        }

        if (!face->send(std::move(interests))) {
            LOG_FATAL("unable to send Interest packets on face");
            this->end();
        }
    }
}

void PipelineInterestsAimd::onData(std::shared_ptr<ndn::Data> &&data,
                                   ndn::lp::PitToken &&pitToken) {
    auto pitEntry = getPITTokenValue(std::move(pitToken));

    if (m_pit->find(pitEntry) == m_pit->end()) {
        LOG_DEBUG("unexpected Data packet dropped");
        return;
    }

    if (data->getCongestionMark()) {
        decreaseWindow();
        LOG_DEBUG("ECN received");
    }

    enqueueData(std::move(data)); // Enqueue Data into Response queue
    m_pit->erase(pitEntry);
    increaseWindow();
}

void PipelineInterestsAimd::onNack(std::shared_ptr<ndn::lp::Nack> &&nack,
                                   ndn::lp::PitToken &&pitToken) {

    auto pitEntry = getPITTokenValue(std::move(pitToken));
    LOG_INFO(
        "received NACK with reason=%i",
        static_cast<typename std::underlying_type<ndn::lp::NackReason>::type>(
            nack->getReason()));

    switch (nack->getReason()) {
    case ndn::lp::NackReason::NONE:
    case ndn::lp::NackReason::DUPLICATE: {
        auto interest = getWireDecode(m_pit->at(pitEntry).interest);
        interest->refreshNonce();

        uint64_t newPITEntry = m_rdn->get();
        auto lifetime = interest->getInterestLifetime().count();

        auto pendingInterest = PendingInterest(
            std::move(getWireEncode(std::move(interest), newPITEntry)),
            newPITEntry, lifetime);

        m_requestQueue.enqueue(std::move(pendingInterest));
        m_pit->erase(pitEntry);
        break;
    }
    default:
        enqueueData(nullptr); // Enqueue null to mark error
        m_pit->erase(pitEntry);
        break;
    }
}

void PipelineInterestsAimd::onTimeout() {
    while (!m_queue->empty()) {
        auto entry = m_pit->find(m_queue->front());
        if (entry == m_pit->end()) {
            // Pop entries that were already satisfied with success
            m_queue->pop();
            continue;
        }

        if (!entry->second.isExpired()) {
            return;
        }

        m_queue->pop();

        auto interest = getWireDecode(entry->second.interest);
        interest->refreshNonce();

        LOG_DEBUG("timeout (%li): %s", entry->second.nTimeout + 1,
                  interest->getName().toUri().c_str());

        decreaseWindow();

        auto newPITEntry = m_rdn->get();
        auto lifetime = interest->getInterestLifetime().count();

        auto pendingInterest = PendingInterest(
            std::move(getWireEncode(std::move(interest), newPITEntry)),
            newPITEntry, lifetime);
        pendingInterest.nTimeout = entry->second.nTimeout + 1;

        if (pendingInterest.nTimeout < 8) {
            m_pit->erase(entry);
            m_requestQueue.enqueue(std::move(pendingInterest));
        } else {
            enqueueData(nullptr); // Enqueue null to mark error
            m_pit->erase(entry);
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
    if (m_windowSize < MIN_WINDOW) {
        m_windowSize = MIN_WINDOW;
    }
    m_lastDecrease = now;
}

void PipelineInterestsAimd::increaseWindow() {
    m_windowSize++;
    if (m_windowSize > MAX_WINDOW) {
        m_windowSize = MAX_WINDOW;
    }
}

}; // namespace ndnc
