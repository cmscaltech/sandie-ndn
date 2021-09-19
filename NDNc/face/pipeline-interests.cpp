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

#include "pipeline-interests.hpp"

namespace ndnc {
Pipeline::Pipeline(Face &face)
    : PacketHandler(face), m_shouldStop{false}, m_maxFixedPipeSize{256},
      m_timeoutQueue{} {

    m_pitGen = std::make_shared<ndnc::lp::PitTokenGenerator>();
    m_workerT = std::thread(&Pipeline::run, this);
}

Pipeline::~Pipeline() {
    this->stop();

    if (m_workerT.joinable()) {
        m_workerT.join();
    }
}

bool Pipeline::enqueueInterestPacket(
    const std::shared_ptr<const ndn::Interest> &interest, void *rxQueue) {

    return m_txQueue.enqueue(
        PendingTask(std::move(interest), static_cast<RxQueue *>(rxQueue)));
}

void Pipeline::dequeueDataPacket(const std::shared_ptr<const ndn::Data> &data,
                                 const ndn::lp::PitToken &pitToken) {

    auto value = lp::PitTokenGenerator::getValue(pitToken.data());

    if (m_pitTable.find(value) == m_pitTable.end()) {
        std::cout << "DEBUG: drop unexpected Data packet\n";
        return;
    }

    m_pitTable[value]->enqueue(TaskResult(std::move(data)));
    m_pitTable.erase(value);
}

void Pipeline::dequeueNackPacket(
    const std::shared_ptr<const ndn::lp::Nack> &nack,
    const ndn::lp::PitToken &pitToken) {

    auto token = lp::PitTokenGenerator::getValue(pitToken.data());

    switch (nack->getReason()) {
    case ndn::lp::NackReason::DUPLICATE: {
        this->enqueueInterestPacket(
            std::make_shared<const ndn::Interest>(nack->getInterest()),
            m_pitTable[token]);
        break;
    }
    default:
        m_pitTable[token]->enqueue(TaskResult(true));
        break;
    }

    m_pitTable.erase(token);
}

void Pipeline::run() {
    if (m_face == nullptr) {
        std::cout << "FATAL: face object is null\n";
        this->stop();
        return;
    }

    while (this->isValid() && m_face != nullptr) {
        // Handle Interests timeout
        m_face->loop();
        while (!m_timeoutQueue.empty()) {
            auto task = m_timeoutQueue.top();

            if (m_pitTable.find(task.pitTokenValue) == m_pitTable.end()) {
                m_timeoutQueue.pop();
                continue;
            }

            if (!task.hasExpired()) {
                break;
            }

            m_timeoutQueue.pop();
            ++task.nTimeouts;

            std::cout << "DEBUG: Interest: " << task.interest->getName()
                      << " timeout count: " << task.nTimeouts << "\n";

            if (task.nTimeouts < 8) { // hardcoded value for max timeout retries
                m_txQueue.enqueue(task);
            } else {
                task.rxQueue->enqueue(TaskResult(true));
                m_pitTable.erase(task.pitTokenValue);
            }
        }

        // Process main queue
        m_face->loop();
        PendingTask task;
        if (m_pitTable.size() > this->m_maxFixedPipeSize || // fixed window size
            !m_txQueue.try_dequeue(task)) {
            continue;
        }

        uint64_t value;
        auto token = m_pitGen->getToken(value);

        if (m_face->expressInterest(std::move(task.interest),
                                    std::move(token))) {
            m_pitTable[value] = task.rxQueue;

            task.pitTokenValue = value;
            task.updateExpirationTime();
            m_timeoutQueue.push(task);
        } else {
            std::cout << "FATAL: unable to express Interest on face\n";
            this->stop();
        }
    }
}
}; // namespace ndnc
