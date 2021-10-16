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

#include <iostream>

#include "pipeline-interests-fixed.hpp"

namespace ndnc {
PipelineInterestsFixed::PipelineInterestsFixed(Face &face, size_t size)
    : PipelineInterests(face), m_maxSize{size} {}

PipelineInterestsFixed::~PipelineInterestsFixed() {
    this->end();
}

void PipelineInterestsFixed::process() {
    while (this->isValid()) {
        face->loop();
        onTimeout();

        if (m_pit->size() >= m_maxSize) {
            continue;
        }

#ifdef DEBUG
        assert(m_pit->size() <= m_maxSize);
#endif

        std::vector<PendingInterest> pendingInterests(m_maxSize -
                                                      m_pit->size());
        size_t n = m_requestQueue.try_dequeue_bulk(pendingInterests.begin(),
                                                   m_maxSize - m_pit->size());

        if (n == 0) {
            continue;
        }

        std::vector<ndn::Block> interests;
        interests.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            auto key = pendingInterests[i].pitEntry;

            m_pit->emplace(std::make_pair(key, pendingInterests[i]));
            interests.emplace(interests.end(),
                              std::move(pendingInterests[i].interest));

            m_queue->push(key);
            m_pit->at(key).markAsExpressed();
        }

        if (!face->send(std::move(interests))) {
            std::cout << "FATAL: unable to send Interest packets on face\n";
            this->end();
        }
    }
}

void PipelineInterestsFixed::onData(std::shared_ptr<ndn::Data> &&data,
                                    ndn::lp::PitToken &&pitToken) {
    auto pitEntry = getPITTokenValue(std::move(pitToken));

    if (m_pit->find(pitEntry) == m_pit->end()) {
        std::cout << "DEBUG: unexpected Data packet dropped\n";
        return;
    }

    enqueueData(std::move(data)); // Enqueue Data into Response queue
    m_pit->erase(pitEntry);
}

void PipelineInterestsFixed::onNack(std::shared_ptr<ndn::lp::Nack> &&nack,
                                    ndn::lp::PitToken &&pitToken) {

    auto pitEntry = getPITTokenValue(std::move(pitToken));
    std::cout << "INFO: received NACK with reason " << nack->getReason()
              << "\n";

    switch (nack->getReason()) {
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

void PipelineInterestsFixed::onTimeout() {
    while (!m_queue->empty()) {
        if (m_pit->count(m_queue->front()) == 0) {
            // Pop entries that were already satisfied with success
            m_queue->pop();
            continue;
        }

        auto entry = m_pit->find(m_queue->front());
        if (!entry->second.isExpired()) {
            return;
        }

        m_queue->pop();

        auto interest = getWireDecode(entry->second.interest);
        interest->refreshNonce();

        std::cout << "TRACE: timeout(" << entry->second.nTimeout + 1 << ") "
                  << interest->getName() << " \n";

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
}; // namespace ndnc
