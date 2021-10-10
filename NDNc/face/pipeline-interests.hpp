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

#ifndef NDNC_FACE_PIPELINE_INTERESTS_HPP
#define NDNC_FACE_PIPELINE_INTERESTS_HPP

#include <chrono>
#include <thread>

#include "concurrentqueue/blockingconcurrentqueue.h"
#include "concurrentqueue/concurrentqueue.h"

#include "encoding/encoding.hpp"
#include "packet-handler.hpp"
#include "utils/random-number-generator.hpp"

namespace ndnc {
enum PendingInterestResultError { NONE = 0, NETWORK = 1 };

class PendingInterestResult {
  public:
    PendingInterestResult() {}

    PendingInterestResult(PendingInterestResultError errCode)
        : errCode(errCode) {}

    PendingInterestResult(std::shared_ptr<ndn::Data> &&data)
        : data{std::move(data)}, errCode(NONE) {}

    auto getData() { return this->data; }
    auto getErrorCode() { return this->errCode; }
    auto hasError() { return this->errCode != NONE; }

  private:
    std::shared_ptr<ndn::Data> data;
    PendingInterestResultError errCode;
};
// pipeline -> worker
typedef moodycamel::BlockingConcurrentQueue<PendingInterestResult> RxQueue;
}; // namespace ndnc

namespace ndnc {
class PendingInterest {
  public:
    PendingInterest() = default;

    PendingInterest(RxQueue *rxQueue, ndn::Block &&interest, uint64_t pitKey)
        : pitKey(pitKey), nTimeout(0), interest(std::move(interest)),
          rxQueue(rxQueue) {}

    ~PendingInterest() = default;

    void markAsExpressed() {
        this->expressedTimePoint = std::chrono::high_resolution_clock::now();
    }

    bool isExpired() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() -
                   this->expressedTimePoint)
                   .count() >
               2000; // TODO: interest->getInterestLifetime().count();
    }

  public:
    uint64_t pitKey;
    uint64_t nTimeout;

    std::chrono::time_point<std::chrono::high_resolution_clock>
        expressedTimePoint;

    ndn::Block interest;
    RxQueue *rxQueue;
};
// worker -> pipeline
typedef moodycamel::ConcurrentQueue<PendingInterest> TxQueue;
}; // namespace ndnc

namespace ndnc {
enum PipelineType {
    fixed = 2,
    // TBD: cubic
    undefined
};

class Pipeline : public PacketHandler {
  public:
    explicit Pipeline(Face &face) : PacketHandler(face), m_stop{false} {
        m_rdn = std::make_shared<RandomNumberGenerator>();
    }

    virtual ~Pipeline() { this->stop(); }

    void start() { m_sender = std::thread(&Pipeline::run, this); }

    void stop() {
        this->m_stop = true;
        if (m_sender.joinable()) {
            m_sender.join();
        }
    }

    virtual bool isValid() { return !this->m_stop && m_face != nullptr; }

  private:
    virtual void run() = 0;

  public:
    bool enqueueInterestPacket(std::shared_ptr<ndn::Interest> &&interest,
                               void *rxQueue) = 0;

    bool
    enqueueInterests(std::vector<std::shared_ptr<ndn::Interest>> &&interests,
                     size_t n, void *rxQueue) = 0;

    void dequeueDataPacket(std::shared_ptr<ndn::Data> &&data,
                           ndn::lp::PitToken &&pitToken) = 0;

    void dequeueNackPacket(std::shared_ptr<ndn::lp::Nack> &&nack,
                           ndn::lp::PitToken &&pitToken) = 0;

  public:
    std::shared_ptr<RandomNumberGenerator> m_rdn;

  private:
    std::thread m_sender;
    std::atomic_bool m_stop;
};
}; // namespace ndnc

#endif // NDNC_FACE_PIPELINE_INTERESTS_HPP
