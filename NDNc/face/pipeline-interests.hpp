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

#include "lp/pit-token.hpp"
#include "packet-handler.hpp"

namespace ndnc {
class TaskResult {
  public:
    TaskResult() {}
    TaskResult(bool error) : error(error) {}
    TaskResult(const std::shared_ptr<const ndn::Data> &data)
        : data(data), error(false) {}

    TaskResult(const std::shared_ptr<const ndn::Data> &data, bool error)
        : data(data), error(error) {}

    auto getData() { return this->data; }
    auto isError() { return this->error; }

  private:
    std::shared_ptr<const ndn::Data> data;
    bool error;
};
// pipeline -> worker
typedef moodycamel::ConcurrentQueue<TaskResult> RxQueue;
}; // namespace ndnc

namespace ndnc {
class PendingTask {
  public:
    PendingTask() : rxQueue(nullptr), expirationTime{0}, nTimeout{0} {}

    PendingTask(const std::shared_ptr<const ndn::Interest> &interest,
                RxQueue *rxQueue)
        : interest(interest), expirationTime{0}, nTimeout{0} {
        this->rxQueue = rxQueue;
    }

    ~PendingTask() {}

    void markAsExpressed() {
        this->expirationTime =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count() +
            interest->getInterestLifetime().count();
    }

    bool expired() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count() > static_cast<int64_t>(expirationTime);
    }

  public:
    std::shared_ptr<const ndn::Interest> interest;
    RxQueue *rxQueue;

    uint64_t expirationTime;
    uint64_t nTimeout;
};
// worker -> pipeline
typedef moodycamel::ConcurrentQueue<PendingTask> TxQueue;

struct GreaterThanByExpirationTime {
    bool operator()(PendingTask lhs, PendingTask rhs) const {
        return lhs.expirationTime > rhs.expirationTime;
    }
};
}; // namespace ndnc

namespace ndnc {
enum PipelineType {
    fixed = 2,
    // TBD: cubic
    undefined
};

class Pipeline : public PacketHandler {
  public:
    explicit Pipeline(Face &face) : PacketHandler(face) {
        m_pitTokenGen = std::make_shared<RandomNumberGenerator>();
        m_shouldStop = false;

        m_worker = std::thread(&Pipeline::run, this);
    }

    virtual ~Pipeline() { this->stop(); }

    void stop() {
        this->m_shouldStop = true;
        if (m_worker.joinable()) {
            m_worker.join();
        }
    }

    virtual bool isValid() { return !this->m_shouldStop; }

  private:
    virtual void run() = 0;

  public:
    bool
    enqueueInterestPacket(const std::shared_ptr<const ndn::Interest> &interest,
                          void *rxQueue) = 0;

    void dequeueDataPacket(const std::shared_ptr<const ndn::Data> &data,
                           const ndn::lp::PitToken &pitToken) = 0;

    void dequeueNackPacket(const std::shared_ptr<const ndn::lp::Nack> &nack,
                           const ndn::lp::PitToken &pitToken) = 0;

  public:
    std::shared_ptr<RandomNumberGenerator> m_pitTokenGen;

  private:
    std::thread m_worker;
    std::atomic_bool m_shouldStop;
};
}; // namespace ndnc

#endif // NDNC_FACE_PIPELINE_INTERESTS_HPP
