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
    : Pipeline(face), m_maxFixedPipeSize{size} {}

PipelineFixed::~PipelineFixed() {
    this->stop();
}

bool PipelineFixed::enqueueInterestPacket(
    const std::shared_ptr<const ndn::Interest> &interest, void *rxQueue) {
    return m_tasksQueue.enqueue(
        PendingTask(std::move(interest), static_cast<RxQueue *>(rxQueue)));
}

void PipelineFixed::dequeueDataPacket(
    const std::shared_ptr<const ndn::Data> &data,
    const ndn::lp::PitToken &pitToken) {

    auto pitKey = lp::getPITTokenValue(pitToken.data());

    if (m_pit.find(pitKey) == m_pit.end()) {
        std::cout << "DEBUG: unexpected Data packet dropped\n";
        return;
    }

    replyWithData(std::move(data), pitKey);
}

void PipelineFixed::dequeueNackPacket(
    const std::shared_ptr<const ndn::lp::Nack> &nack,
    const ndn::lp::PitToken &pitToken) {

    auto pitKey = lp::getPITTokenValue(pitToken.data());

    switch (nack->getReason()) {
    case ndn::lp::NackReason::DUPLICATE: {
        auto rxQueue = m_pit[pitKey].rxQueue;
        m_pit.erase(pitKey);

        this->enqueueInterestPacket(
            std::make_shared<const ndn::Interest>(nack->getInterest()),
            rxQueue);
        return;
    }
    default:
        replyWithError(pitKey);
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

        if (m_pit.size() == m_maxFixedPipeSize) {
            continue;
        } else {
            // DEBUG
            assert(m_pit.size() < m_maxFixedPipeSize);
        }

        std::vector<PendingTask> tasks(m_maxFixedPipeSize - m_pit.size());
        size_t n = m_tasksQueue.try_dequeue_bulk(
            tasks.begin(), m_maxFixedPipeSize - m_pit.size());

        if (n == 0) {
            continue;
        } else if (n == 1) {
            handleTask(tasks[0]);
        } else {
            handleTasks(tasks, n);
            tasks.clear();
        }
    }
}

void PipelineFixed::handleTask(PendingTask task) {
    auto pitKey = m_pitTokenGen->getNext();

    if (m_face->expressInterest(std::move(task.interest), pitKey)) {
        task.markAsExpressed();
        m_pit[pitKey] = task;
    } else {
        std::cout << "FATAL: unable to express Interest on face\n";
        this->stop();
    }
}

void PipelineFixed::handleTasks(std::vector<PendingTask> tasks, size_t n) {
    std::vector<std::shared_ptr<const ndn::Interest>> interests;
    std::vector<uint64_t> pitTokens;

    for (size_t i = 0; i < n; ++i) {
        auto pitKey = m_pitTokenGen->getNext();

        interests.push_back(tasks[i].interest);
        pitTokens.push_back(pitKey);
    }

    if (!m_face->expressInterests(interests, pitTokens)) {
        std::cout << "FATAL: unable to express Interests on face\n";
        this->stop();
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        tasks[i].markAsExpressed();
        m_pit[pitTokens[i]] = tasks[i];
    }
}

void PipelineFixed::handleTimeout() {
    // Check for timeout one Interest per iteration.
    // PIT table is a ordered map with PIT TOKEN value as key, which is
    // generated sequentially by RandomNumberGenerator, thus first entry is the
    // oldest expressed Interest packet
    if (m_pit.size() == 0 || !m_pit.begin()->second.expired()) {
        return;
    }

    auto pendingTask = m_pit.begin()->second;

    pendingTask.nTimeout += 1;
    std::cout << "TRACE: Request timeout for "
              << pendingTask.interest->getName() << " " << pendingTask.nTimeout
              << "\n";

    if (pendingTask.nTimeout < 8) {
        m_pit.erase(m_pit.begin()->first);
        handleTask(pendingTask);
    } else {
        replyWithError(m_pit.begin()->first);
    }
}

void PipelineFixed::replyWithError(uint64_t pitTokenValue) {
    m_pit[pitTokenValue].rxQueue->enqueue(TaskResult(true));
    m_pit.erase(pitTokenValue);
}

void PipelineFixed::replyWithData(const std::shared_ptr<const ndn::Data> &data,
                                  uint64_t pitTokenValue) {
    m_pit[pitTokenValue].rxQueue->enqueue(TaskResult(std::move(data)));
    m_pit.erase(pitTokenValue);
}
}; // namespace ndnc
