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

#ifndef NDNC_PIPELINE_INTERESTS_HPP
#define NDNC_PIPELINE_INTERESTS_HPP

#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "face/packet-handler.hpp"
#include "pending-interest.hpp"
#include "pipeline-type.hpp"
#include "utils/random-number-generator.hpp"

namespace ndnc {
class PipelineInterests : public PacketHandler {
  public:
    using PendingInterestsTable = std::unordered_map<uint64_t, PendingInterest>;
    using ExpressedInterestsQueue = std::queue<uint64_t>;

  public:
    explicit PipelineInterests(Face &face) : PacketHandler(face), m_stop{true} {
        m_pit = std::make_shared<PendingInterestsTable>();
        m_queue = std::make_shared<ExpressedInterestsQueue>();
        m_rdn = std::make_shared<RandomNumberGenerator>();
    }

    virtual ~PipelineInterests() {
        this->end();

        m_pit->clear();
        while (!m_queue->empty()) {
            m_queue->pop();
        }
    }

    void begin() {
        this->m_stop = false;
        this->m_sender = std::thread(&PipelineInterests::process, this);
    }

    void end() {
        this->m_stop = true;
        if (this->m_sender.joinable()) {
            this->m_sender.join();
        }
    }

    bool isValid() { return !this->m_stop && face != nullptr; }

    uint64_t size() { return m_pit->size(); }

  public:
    bool enqueueInterest(std::shared_ptr<ndn::Interest> &&interest) {
        if (!isValid()) {
            return false;
        }

        uint64_t pitEntry = m_rdn->get();
        auto lifetime = interest->getInterestLifetime().count();

        auto pendingInterest = PendingInterest(
            std::move(getWireEncode(std::move(interest), pitEntry)), pitEntry,
            lifetime);

        return m_requestQueue.enqueue(std::move(pendingInterest));
    }

    bool
    enqueueInterests(std::vector<std::shared_ptr<ndn::Interest>> &&interests) {
        if (!isValid()) {
            return false;
        }

        std::vector<PendingInterest> pendingInterests;
        pendingInterests.reserve(interests.size());

        for (size_t i = 0; i < interests.size(); ++i) {
            auto pitEntry = m_rdn->get();
            auto lifetime = interests[i]->getInterestLifetime().count();

            pendingInterests.emplace_back(PendingInterest(
                std::move(getWireEncode(std::move(interests[i]), pitEntry)),
                pitEntry, lifetime));
        }

        return m_requestQueue.enqueue_bulk(pendingInterests.begin(),
                                           pendingInterests.size());
    }

    bool dequeueData(std::shared_ptr<ndn::Data> &data) {
        return m_responseQueue.wait_dequeue_timed(data, 1000);
    }

  protected:
    bool enqueueData(std::shared_ptr<ndn::Data> &&data) {
        if (!isValid()) {
            return false;
        }

        return m_responseQueue.enqueue(std::move(data));
    }

  private:
    virtual void process() = 0;
    virtual void onTimeout() = 0;

  public:
    std::shared_ptr<PendingInterestsTable> m_pit;
    std::shared_ptr<ExpressedInterestsQueue> m_queue;
    std::shared_ptr<RandomNumberGenerator> m_rdn;

    RequestQueue m_requestQueue;
    ResponseQueue m_responseQueue;

  private:
    std::atomic_bool m_stop;
    std::thread m_sender;
};
}; // namespace ndnc

#endif // NDNC_PIPELINE_INTERESTS_HPP
