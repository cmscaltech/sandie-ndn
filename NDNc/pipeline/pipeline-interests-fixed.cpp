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

#include "face/memif-constants.hpp"
#include "logger/logger.hpp"
#include "pipeline-interests-fixed.hpp"

namespace ndnc {
PipelineInterestsFixed::PipelineInterestsFixed(Face &face, size_t size)
    : PipelineInterests(face), m_maxWindowSize{size} {}

PipelineInterestsFixed::~PipelineInterestsFixed() {
    this->end();
}

void PipelineInterestsFixed::process() {
    while (this->isValid()) {
        face->loop();
        onTimeout();

        if (m_pit->size() >= m_maxWindowSize) {
            continue;
        }

        size_t n = std::min(static_cast<int>(m_maxWindowSize - m_pit->size()),
                            MAX_MEMIF_BUFS);

        std::vector<PendingInterest> pendingInterests(n);
        n = m_requestQueue.try_dequeue_bulk(pendingInterests.begin(), n);

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

void PipelineInterestsFixed::onData(std::shared_ptr<ndn::Data> &&data,
                                    ndn::lp::PitToken &&pitToken) {
    auto pitEntry = getPITTokenValue(std::move(pitToken));

    if (m_pit->find(pitEntry) == m_pit->end()) {
        LOG_DEBUG("unexpected Data packet dropped");
        return;
    }

    enqueueData(std::move(data)); // Enqueue Data into Response queue
    m_pit->erase(pitEntry);
}

void PipelineInterestsFixed::onNack(std::shared_ptr<ndn::lp::Nack> &&nack,
                                    ndn::lp::PitToken &&pitToken) {

    auto pitEntry = getPITTokenValue(std::move(pitToken));
    LOG_INFO(
        "received NACK with reason=%i",
        static_cast<typename std::underlying_type<ndn::lp::NackReason>::type>(
            nack->getReason()));

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
