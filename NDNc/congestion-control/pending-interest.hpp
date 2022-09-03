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

#ifndef NDNC_CONGESTION_CONTROL_PIPELINE_PENDING_INTEREST_HPP
#define NDNC_CONGESTION_CONTROL_PIPELINE_PENDING_INTEREST_HPP

#include <ndn-cxx/util/time.hpp>

#include "pipeline-common.hpp"

namespace ndnc {
class PendingInterest {
  public:
    PendingInterest() {
    }

    PendingInterest(std::shared_ptr<ndn::Interest> &&interest,
                    uint64_t pitTokenValue, uint64_t consumerId) {
        m_pitTokenValue = pitTokenValue;
        m_consumerId = consumerId;
        m_retriesCount = 0;
        m_interestLifetime = interest->getInterestLifetime();
        m_interest = getWireEncode(std::move(interest), pitTokenValue);
    }

    ~PendingInterest() {
    }

    uint64_t getPITTokenValue() {
        return m_pitTokenValue;
    }

    uint64_t getConsumerId() {
        return m_consumerId;
    }

    uint64_t getRetriesCount() {
        return m_retriesCount;
    }

    bool hasReachedMaximumNumOfRetries() {
        return m_retriesCount >= 8;
    }

    std::shared_ptr<ndn::Interest> getInterest() {
        return getWireDecode(this->m_interest);
    }

    ndn::Block getInterestBlockValue() {
        return m_interest;
    }

    bool isExpired() const {
        return getTimeSinceExpressed() > m_interestLifetime;
    }

    void markAsExpressed() {
        expressedAt = ndn::time::steady_clock::now();
    }

    inline ndn::time::milliseconds getTimeSinceExpressed() const {
        return ndn::time::duration_cast<ndn::time::milliseconds>(
            ndn::time::steady_clock::now() - expressedAt);
    }

    void refresh(uint64_t pitTokenValue, bool timeoutReason) {
        auto interest = this->getInterest();
        interest->refreshNonce();

        this->m_pitTokenValue = pitTokenValue;
        this->m_interest = getWireEncode(std::move(interest), pitTokenValue);

        if (timeoutReason) {
            this->m_retriesCount += 1;
        }
    }

  private:
    uint64_t m_pitTokenValue;
    uint64_t m_consumerId;
    uint64_t m_retriesCount;
    ndn::Block m_interest;
    ndn::time::milliseconds m_interestLifetime;
    ndn::time::steady_clock::TimePoint expressedAt;
};
}; // namespace ndnc

#endif // NDNC_CONGESTION_CONTROL_PIPELINE_PENDING_INTEREST_HPP
