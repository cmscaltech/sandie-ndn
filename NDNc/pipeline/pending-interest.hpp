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

#ifndef NDNC_PENDING_INTEREST_HPP
#define NDNC_PENDING_INTEREST_HPP

#include <ndn-cxx/util/time.hpp>

#include "concurrentqueue/blockingconcurrentqueue.h"
#include "concurrentqueue/concurrentqueue.h"
#include "encoding/encoding.hpp"

namespace ndnc {
class PendingInterest {
  public:
    PendingInterest() {
    }

    PendingInterest(std::shared_ptr<ndn::Interest> &&interest,
                    uint64_t pitTokenValue, uint64_t timeoutCounter = 0)
        : pitTokenValue{pitTokenValue}, timeoutCounter{timeoutCounter} {

        interestLifetime = interest->getInterestLifetime();
        interestBlockValue = getWireEncode(std::move(interest), pitTokenValue);
    }

    ~PendingInterest() {
    }

    void markAsExpressed() {
        expressedAt = ndn::time::steady_clock::now();
    }

    inline ndn::time::milliseconds timeSinceExpressed() const {
        return ndn::time::duration_cast<ndn::time::milliseconds>(
            ndn::time::steady_clock::now() - expressedAt);
    }

    bool hasExpired() const {
        return timeSinceExpressed() > interestLifetime;
    }

  public:
    uint64_t pitTokenValue;
    uint64_t timeoutCounter;

    ndn::Block interestBlockValue;

  private:
    ndn::time::milliseconds interestLifetime;
    ndn::time::steady_clock::TimePoint expressedAt;
};

typedef moodycamel::ConcurrentQueue<PendingInterest> RequestQueue;
typedef moodycamel::BlockingConcurrentQueue<std::shared_ptr<ndn::Data>>
    ResponseQueue;
}; // namespace ndnc

#endif // NDNC_PENDING_INTEREST_HPP
