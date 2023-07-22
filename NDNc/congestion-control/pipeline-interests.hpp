/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2023 California Institute of Technology
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

#ifndef NDNC_CONGESTION_CONTROL_PIPELINE_INTERESTS_HPP
#define NDNC_CONGESTION_CONTROL_PIPELINE_INTERESTS_HPP

#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "face/packet-handler.hpp"
#include "pending-interest.hpp"
#include "pipeline-type.hpp"
#include "utils/threadsafe-uint64-generator.hpp"

namespace ndnc {
struct PipelineCounters {
    ndn::time::milliseconds delay{0};
    uint64_t nack = 0;
    uint64_t timeout = 0;
    uint64_t tx = 0;
    uint64_t rx = 0;
    uint64_t rxUnexpected = 0;

    ndn::time::milliseconds getAverageDelay() {
        return ndn::time::milliseconds{rx > 0 ? delay.count() / rx : 0};
    }
};

class PipelineInterests : public PacketHandler {
  private:
    using PendingInterestsOrder = std::queue<uint64_t>;
    using PendingInterestsTable = std::unordered_map<uint64_t, PendingInterest>;
    using ResponseQueuesMap = std::unordered_map<uint64_t, ResponseQueue>;

  public:
    explicit PipelineInterests(face::Face &face)
        : PacketHandler(face), m_counters{}, m_closed{false} {

        face.addOnDisconnectHandler([&]() { this->m_closed = true; });

        m_pit = std::make_shared<PendingInterestsTable>();
        m_piq = std::make_shared<PendingInterestsOrder>();
        m_rdn = std::make_shared<ThreadSafeUInt64Generator>();

        registerConsumer(0);
        m_worker = std::thread(&PipelineInterests::open, this);
    }

    virtual ~PipelineInterests() {
        this->close();

        if (m_worker.joinable()) {
            m_worker.join();
        }

        m_pit->clear();

        while (!m_piq->empty()) {
            m_piq->pop();
        }
    }

    void close() {
        if (!isClosed()) {
            m_closed = true;
        }
    }

    bool isClosed() {
        return m_closed;
    }

    PipelineCounters getCounters() {
        return m_counters;
    }

    uint64_t getQueuedInterestsCount() {
        return m_requestQueue.size_approx();
    }

    uint64_t registerConsumer() {
        return registerConsumer(m_rdn->generate());
    }

    uint64_t registerConsumer(const uint64_t consumerId) {
        std::lock_guard<std::mutex> lock(m_responseQueuesMtx);
        responseQueuesMap_[consumerId] = ResponseQueue();
        return consumerId;
    }

    void unregisterConsumer(const uint64_t consumerId) {
        std::lock_guard<std::mutex> lock(m_responseQueuesMtx);
        responseQueuesMap_.erase(consumerId);
    }

    bool pushInterest(uint64_t consumerId,
                      std::shared_ptr<ndn::Interest> &&pkt) {
        // Do nothing if the pipeline is already closed
        if (isClosed()) {
            LOG_INFO("pipeline is closed (push interest)");
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(m_responseQueuesMtx);
            if (responseQueuesMap_.find(consumerId) ==
                responseQueuesMap_.end()) {
                LOG_WARN("unable to push interest pkt. reason: unregistered "
                         "consumer id=%lu",
                         consumerId);
                close();
                return false;
            }
        }

        auto newPendingInterest =
            PendingInterest(std::move(pkt), m_rdn->generate(), consumerId);

        return m_requestQueue.enqueue(std::move(newPendingInterest));
    }

    bool pushInterestBulk(uint64_t consumerId,
                          std::vector<std::shared_ptr<ndn::Interest>> &&pkts) {
        // Do nothing if the pipeline is already closed
        if (isClosed()) {
            LOG_INFO("pipeline is closed (push interest bulk)");
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(m_responseQueuesMtx);
            if (responseQueuesMap_.find(consumerId) ==
                responseQueuesMap_.end()) {
                LOG_ERROR("unable to push interest pkts. reason: unregistered "
                          "consumer id=%ld",
                          consumerId);
                close();
                return false;
            }
        }

        std::vector<PendingInterest> newPendingInterests;
        newPendingInterests.reserve(pkts.size());

        for (uint64_t i = 0; i < pkts.size(); ++i) {
            auto newPendingInterest = PendingInterest(
                std::move(pkts[i]), m_rdn->generate(), consumerId);
            newPendingInterests.emplace_back(std::move(newPendingInterest));
        }

        return m_requestQueue.enqueue_bulk(newPendingInterests.begin(),
                                           newPendingInterests.size());
    }

    bool popData(uint64_t consumerId, std::shared_ptr<ndn::Data> &pkt) {
        // Do nothing if the pipeline is already closed
        if (isClosed()) {
            LOG_INFO("pipeline is closed (pop data)");
            return false;
        }

        std::lock_guard<std::mutex> lock(m_responseQueuesMtx);

        try {
            return responseQueuesMap_.at(consumerId).try_dequeue(pkt);
        } catch (const std::out_of_range &oor) {
            LOG_ERROR("out of range error (pop data): %s for consumer id: %ld",
                      oor.what(), consumerId);
            close();
            return false;
        }
    }

    size_t popDataBulk(uint64_t consumerId,
                       std::vector<std::shared_ptr<ndn::Data>> &pkts) {
        // Do nothing if the pipeline is already closed
        if (isClosed()) {
            LOG_INFO("pipeline is closed (pop data bulk)");
            return 0;
        }

        std::lock_guard<std::mutex> lock(m_responseQueuesMtx);

        try {
            return responseQueuesMap_.at(consumerId)
                .try_dequeue_bulk(pkts.begin(), pkts.size());
        } catch (const std::out_of_range &oor) {
            LOG_ERROR("out of range error (pop data bulk): %s for consumer "
                      "id: %ld",
                      oor.what(), consumerId);
            close();
            return 0;
        }
    }

  protected:
    bool pushData(uint64_t consumerId, std::shared_ptr<ndn::Data> &&pkt) {
        if (isClosed()) {
            LOG_INFO("pipeline is closed (push data)");
            return false;
        }

        std::lock_guard<std::mutex> lock(m_responseQueuesMtx);

        try {
            return responseQueuesMap_.at(consumerId).enqueue(std::move(pkt));
        } catch (const std::out_of_range &oor) {
            LOG_ERROR("out of range error (push data): %s for consumer id: %ld",
                      oor.what(), consumerId);
            close();
            return false;
        }
    }

    size_t popPendingInterests(std::vector<PendingInterest> &pendingInterests,
                               size_t n) {
        if (isClosed()) {
            LOG_INFO("pipeline is closed (pop pending interests)");
            return 0;
        }

        pendingInterests.clear();
        pendingInterests.reserve(n);

        return m_requestQueue.try_dequeue_bulk(pendingInterests.begin(), n);
    }

    bool refreshPITEntry(uint64_t key, bool timeoutReason = false) {
        try {
            auto pendingInterest = m_pit->at(key);
            pendingInterest.refresh(m_rdn->generate(), timeoutReason);

            if (isClosed()) {
                return false;
            }

            m_pit->erase(key);
            return m_requestQueue.enqueue(std::move(pendingInterest));

        } catch (const std::out_of_range &oor) {
            LOG_ERROR("out of range error (refresh pit): %s", oor.what());
            close();
            return false;
        }

        return true;
    }

  private:
    virtual void open() = 0;
    virtual void onTimeout() = 0;

  public:
    std::shared_ptr<PendingInterestsTable> m_pit;
    std::shared_ptr<PendingInterestsOrder> m_piq;
    std::shared_ptr<ThreadSafeUInt64Generator> m_rdn;
    PipelineCounters m_counters;

  private:
    RequestQueue m_requestQueue;
    ResponseQueuesMap responseQueuesMap_;

    std::mutex m_responseQueuesMtx;
    std::atomic_bool m_closed;
    std::thread m_worker;
};
}; // namespace ndnc

#endif // NDNC_CONGESTION_CONTROL_PIPELINE_INTERESTS_HPP
