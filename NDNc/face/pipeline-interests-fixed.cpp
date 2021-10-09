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

PipelineFixed::PipelineFixed(Face &face, size_t size)
    : Pipeline(face), m_maxSize{size}, m_pit{} {}

PipelineFixed::~PipelineFixed() {
    this->stop();
}

void PipelineFixed::run() {
    while (isValid()) {
        if (m_pit.size() == m_maxSize) {
            m_face->loop();

            processTimeout();
            continue;
        } else {
            // DEBUG
            assert(m_pit.size() < m_maxSize);
        }

        std::vector<PendingInterest> pendingInterests(m_maxSize - m_pit.size());
        // pendingInterests.reserve(m_maxSize - m_pit.size());
        // why doesn't this work ?

        size_t n = m_tasksQueue.try_dequeue_bulk(pendingInterests.begin(),
                                                 m_maxSize - m_pit.size());

        if (n > 0) {
            processInterests(std::move(pendingInterests), n);
        }

        m_face->loop();
        processTimeout();
    }
}

void PipelineFixed::processInterest(PendingInterest &&pendingInterest) {
    std::vector<PendingInterest> pi;
    pi.reserve(1);
    pi.emplace(pi.end(), std::move(pendingInterest));

    this->processInterests(std::move(pi), 1);
}

void PipelineFixed::processInterests(std::vector<PendingInterest> &&pi,
                                     size_t n) {
    std::vector<ndn::Block> request;
    request.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        m_pit.emplace(std::make_pair(pi[i].pitKey, pi[i]));
        request.emplace(request.end(), std::move(pi[i].interest));
    }

    if (!m_face->express(std::move(request))) {
        std::cout << "FATAL: unable to express Interests on face\n";
        this->stop();
    } else {
        for (size_t i = 0; i < n; ++i) {
            m_pit[pi[i].pitKey].markAsExpressed();
        }
    }
}

void PipelineFixed::processTimeout() {
    for (auto it = m_pit.begin(); it != m_pit.end(); ++it) {
        if (it->second.isExpired()) {
            auto interest = getWireDecode(it->second.interest);
            interest->refreshNonce();

            std::cout << "TRACE: timeout(" << it->second.nTimeout + 1 << ") "
                      << interest->getName() << " \n";

            uint64_t pitKey = m_rdn->get();

            auto pendingInterest = PendingInterest(
                it->second.rxQueue,
                std::move(getWireEncode(std::move(interest), pitKey)), pitKey);
            pendingInterest.nTimeout = it->second.nTimeout + 1;

            if (pendingInterest.nTimeout < 8) {
                m_pit.erase(it);
                this->processInterest(std::move(pendingInterest));
            } else {
                replyWithError(NETWORK, it->first);
            }

            return;
        }
    }
}

bool PipelineFixed::enqueueInterestPacket(
    std::shared_ptr<const ndn::Interest> &&interest, void *rxQueue) {
    uint64_t pitKey = m_rdn->get();

    auto pendingInterest = PendingInterest(
        static_cast<RxQueue *>(rxQueue),
        std::move(getWireEncode(std::move(interest), pitKey)), pitKey);

    return m_tasksQueue.enqueue(std::move(pendingInterest));
}

void PipelineFixed::dequeueDataPacket(std::shared_ptr<const ndn::Data> &&data,
                                      ndn::lp::PitToken &&pitToken) {
    auto pitKey = getPITTokenValue(std::move(pitToken));

    if (m_pit.find(pitKey) == m_pit.end()) {
        std::cout << "DEBUG: unexpected Data packet dropped\n";
        return;
    }

    replyWithData(std::move(data), pitKey);
}

void PipelineFixed::dequeueNackPacket(
    std::shared_ptr<const ndn::lp::Nack> &&nack, ndn::lp::PitToken &&pitToken) {

    auto pitKey = getPITTokenValue(std::move(pitToken));

    switch (nack->getReason()) {
    case ndn::lp::NackReason::DUPLICATE: {
        auto interest = getWireDecode(m_pit[pitKey].interest);
        interest->refreshNonce();

        uint64_t newPitKey = m_rdn->get();
        auto pendingInterest = PendingInterest(
            m_pit[pitKey].rxQueue,
            std::move(getWireEncode(std::move(interest), newPitKey)),
            newPitKey);

        m_pit.erase(pitKey);
        this->processInterest(std::move(pendingInterest));
        break;
    }
    default:
        replyWithError(NETWORK, pitKey);
        break;
    }
}

void PipelineFixed::replyWithData(std::shared_ptr<const ndn::Data> &&data,
                                  uint64_t pitKey) {
    if (m_pit[pitKey].rxQueue != nullptr && isValid()) {
        m_pit[pitKey].rxQueue->enqueue(
            std::move(PendingInterestResult(std::move(data))));
    }

    m_pit.erase(pitKey);
}

void PipelineFixed::replyWithError(PendingInterestResultError errCode,
                                   uint64_t pitKey) {
    if (m_pit[pitKey].rxQueue != nullptr && isValid()) {
        m_pit[pitKey].rxQueue->enqueue(
            std::move(PendingInterestResult(errCode)));
    }

    m_pit.erase(pitKey);
}
}; // namespace ndnc
