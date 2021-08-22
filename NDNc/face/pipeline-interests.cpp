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

#include <chrono>
#include <iostream>

#include "pipeline-interests.hpp"

namespace ndnc {
PipelineTask::PipelineTask() {}

PipelineTask::PipelineTask(const std::shared_ptr<const ndn::Interest> &interest,
                           PipelineRxQueue *rxQueue)
    : interest(interest) {
    this->rxQueue = rxQueue;
}

PipelineTask::~PipelineTask() {}

Pipeline::Pipeline(Face &face)
    : PacketHandler(face), m_stop(false), m_hasError(false) {
    m_pitTokenGenerator = std::make_shared<ndnc::lp::PitTokenGenerator>();
    worker = std::thread(&Pipeline::run, this);
}

Pipeline::~Pipeline() {
    this->stop();

    if (worker.joinable()) {
        worker.join();
    }
}

void Pipeline::run() {
    while (!m_stop && !m_hasError) {
        // std::this_thread::sleep_for(std::chrono::nanoseconds(10));
        m_face->loop();

        if (!this->processTxQueue()) {
            m_hasError = true;
            return;
        }
    }
}

void Pipeline::stop() {
    m_stop = true;
}

bool Pipeline::isValid() {
    return !this->m_hasError && !this->m_stop;
}

bool Pipeline::processTxQueue() {
    if (m_face == nullptr) {
        std::cout << "FATAL: face object is null\n";
        m_hasError = true;
        return false;
    }

    PipelineTask task;
    if (!m_txQueue.try_dequeue(task)) {
        return true; // continue
    }

    uint64_t value;
    auto token = m_pitTokenGenerator->getToken(value);

    if (m_pendingInterestsTable.find(value) != m_pendingInterestsTable.end()) {
        std::cout
            << "FATAL: try to express Interest with an existing PIT Token\n";
        m_hasError = true;
        return false;
    }

    m_pendingInterestsTable[value] = task.rxQueue;
    if (!m_face->expressInterest(std::move(task.interest), std::move(token))) {
        std::cout << "FATAL: unable to express Interest on face\n";
        m_hasError = true;
        m_pendingInterestsTable.erase(value);
        return false;
    }

    return true;
}

bool Pipeline::enqueueInterestPacket(
    const std::shared_ptr<const ndn::Interest> &interest, void *rxQueue) {
    if (!m_txQueue.enqueue(PipelineTask(
            std::move(interest), static_cast<PipelineRxQueue *>(rxQueue)))) {
        return false;
    }

    return true;
}

void Pipeline::dequeueDataPacket(const std::shared_ptr<const ndn::Data> &data,
                                 const ndn::lp::PitToken &pitToken) {
    auto value = lp::PitTokenGenerator::getValue(pitToken.data());

    if (m_pendingInterestsTable.find(value) == m_pendingInterestsTable.end()) {
        std::cout << "FATAL: received unexpected Data packet with PIT-TOKEN: "
                  << value << "\n";
        m_hasError = true;
        return;
    }

    m_pendingInterestsTable[value]->enqueue(*data);
    m_pendingInterestsTable.erase(value);
}

void Pipeline::dequeueNackPacket(const std::shared_ptr<const ndn::lp::Nack> &) {
}

void Pipeline::processTimeouts() {}
}; // namespace ndnc
