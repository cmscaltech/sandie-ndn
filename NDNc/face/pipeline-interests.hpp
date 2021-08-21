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

#include <thread>
#include <unordered_map>

#include "concurrentqueue/blockingconcurrentqueue.h"
#include "concurrentqueue/concurrentqueue.h"
#include "lp/pit-token.hpp"
#include "packet-handler.hpp"

namespace ndnc {
typedef moodycamel::BlockingConcurrentQueue<ndn::Data> PipelineRxQueue;

class PipelineTask {
  public:
    PipelineTask();
    PipelineTask(const std::shared_ptr<const ndn::Interest> &interest,
                 PipelineRxQueue *rxQueue);
    ~PipelineTask();

  public:
    std::shared_ptr<const ndn::Interest> interest;
    PipelineRxQueue *rxQueue;
};

typedef moodycamel::ConcurrentQueue<PipelineTask> PipelineTxQueue;

class Pipeline : public PacketHandler {
  public:
    explicit Pipeline(Face &face);
    virtual ~Pipeline();

    virtual void run();
    virtual void stop();
    virtual bool isValid();

  public:
    bool
    enqueueInterestPacket(const std::shared_ptr<const ndn::Interest> &interest,
                          void *rxQueue) final;

    void dequeueDataPacket(const std::shared_ptr<const ndn::Data> &data,
                           const ndn::lp::PitToken &pitToken) final;

    void
    dequeueNackPacket(const std::shared_ptr<const ndn::lp::Nack> &nack) final;

  private:
    virtual bool processTxQueue();
    virtual void processTimeouts();

  private:
    std::thread worker;

    std::atomic_bool m_stop;
    std::atomic_bool m_hasError;
    std::shared_ptr<ndnc::lp::PitTokenGenerator> m_pitTokenGenerator;

    PipelineTxQueue m_txQueue;
    std::unordered_map<uint64_t, PipelineRxQueue *> m_pendingInterestsTable;
};
}; // namespace ndnc

#endif // NDNC_FACE_PIPELINE_INTERESTS_HPP
