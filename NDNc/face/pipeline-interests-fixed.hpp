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

#ifndef NDNC_FACE_PIPELINE_INTERESTS_FIXED_HPP
#define NDNC_FACE_PIPELINE_INTERESTS_FIXED_HPP

#include <map>

#include "pipeline-interests.hpp"

namespace ndnc {
class PipelineFixed : public Pipeline {
  public:
    using PendingInteretsTable = std::map<uint64_t, PendingTask>;

  public:
    PipelineFixed(Face &face, size_t size);
    ~PipelineFixed();

  private:
    virtual void run() final;

    void replyWithError(uint64_t pitTokenValue);
    void replyWithData(const std::shared_ptr<const ndn::Data> &data,
                       uint64_t pitTokenValue);

    void handleTimeout();
    void handleTask(PendingTask task);
    void handleTasks(std::vector<PendingTask> tasks, size_t n);

  public:
    bool
    enqueueInterestPacket(const std::shared_ptr<const ndn::Interest> &interest,
                          void *rxQueue) final;

    void dequeueDataPacket(const std::shared_ptr<const ndn::Data> &data,
                           const ndn::lp::PitToken &pitToken) final;

    void dequeueNackPacket(const std::shared_ptr<const ndn::lp::Nack> &nack,
                           const ndn::lp::PitToken &pitToken) final;

  private:
    size_t m_maxFixedPipeSize;

    TxQueue m_tasksQueue;
    PendingInteretsTable m_pit;
};
}; // namespace ndnc

#endif // NDNC_FACE_PIPELINE_INTERESTS_FIXED_HPP