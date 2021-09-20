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

PipelineFixed::PipelineFixed(Face &face, uint64_t size)
    : Pipeline(face), m_maxFixedPipeSize{size} {

    m_tokenGenerator = std::make_shared<ndnc::lp::PitTokenGenerator>();
    m_pipelineWorker = std::thread(&PipelineFixed::run, this);
}

PipelineFixed::~PipelineFixed() {
    this->stop();

    if (m_pipelineWorker.joinable()) {
        m_pipelineWorker.join();
    }
}

bool PipelineFixed::enqueueInterestPacket(
    const std::shared_ptr<const ndn::Interest> &interest, void *rxQueue) {

    return m_tasksQueue.enqueue(
        PendingTask(std::move(interest), static_cast<RxQueue *>(rxQueue)));
}

void PipelineFixed::dequeueDataPacket(
    const std::shared_ptr<const ndn::Data> &data,
    const ndn::lp::PitToken &pitToken) {

    auto pitEntry = lp::PitTokenGenerator::getValue(pitToken.data());

    if (m_pit.find(pitEntry) == m_pit.end()) {
        std::cout << "DEBUG: unexpected Data packet dropped\n";
        return;
    }

    replyWithData(std::move(data), pitEntry);
}

void PipelineFixed::dequeueNackPacket(
    const std::shared_ptr<const ndn::lp::Nack> &nack,
    const ndn::lp::PitToken &pitToken) {

    auto pitEntry = lp::PitTokenGenerator::getValue(pitToken.data());

    switch (nack->getReason()) {
    case ndn::lp::NackReason::DUPLICATE: {
        this->enqueueInterestPacket(
            std::make_shared<const ndn::Interest>(nack->getInterest()),
            m_pit[pitEntry]);
        m_pit.erase(pitEntry);
        break;
    }
    default:
        replyWithError(pitEntry);
        break;
    }
}

void PipelineFixed::run() {
    if (this->m_face == nullptr) {
        std::cout << "FATAL: face object is null\n";
        this->stop();
        return;
    }

    while (this->isValid() && m_face != nullptr) {
        m_face->loop();
        handleTimeout();
        m_face->loop();

        if (m_pit.size() > m_maxFixedPipeSize) {
            std::cout << "DEBUG: Max pipeline size reached\n";
            continue;
        }

        PendingTask task;
        if (!m_tasksQueue.try_dequeue(task)) {
            continue;
        } else {
            handleTask(task);
        }
    }
}

void PipelineFixed::handleTask(PendingTask task) {
    uint64_t pitEntry;
    auto token = m_tokenGenerator->getToken(pitEntry);
    task.pitEntry = pitEntry;

    if (m_face->expressInterest(std::move(task.interest), std::move(token))) {
        m_pit[task.pitEntry] = task.rxQueue;

        // Keep track of in-flight packets for timeout handling
        task.updateExpirationTime();
        m_timeoutQueue.push(task);
    } else {
        std::cout << "FATAL: unable to express Interest on face\n";
        this->stop();
    }
}

void PipelineFixed::handleTimeout() {
    while (!m_timeoutQueue.empty()) {
        auto task = m_timeoutQueue.top();

        if (m_pit.find(task.pitEntry) == m_pit.end()) {
            m_timeoutQueue.pop();
            continue;
        }

        if (!task.hasExpired()) {
            break;
        } else {
            m_timeoutQueue.pop();
            ++task.nTimeout;
        }

        std::cout << "DEBUG: timeout counter: " << task.nTimeout
                  << " for Interest: " << task.interest->getName() << "\n";

        if (task.nTimeout < 8) { // hardcoded value for max timeout retries
            m_pit.erase(task.pitEntry);
            handleTask(task);
        } else {
            replyWithError(task.pitEntry);
        }
    }
}

void PipelineFixed::replyWithError(uint64_t pitEntry) {
    m_pit[pitEntry]->enqueue(TaskResult(true));
    m_pit.erase(pitEntry);
}

void PipelineFixed::replyWithData(const std::shared_ptr<const ndn::Data> &data,
                                  uint64_t pitEntry) {
    m_pit[pitEntry]->enqueue(TaskResult(std::move(data)));
    m_pit.erase(pitEntry);
}
}; // namespace ndnc
