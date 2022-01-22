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

#include <chrono>
#include <ndn-cxx/data.hpp>

#include "concurrentqueue/blockingconcurrentqueue.h"
#include "concurrentqueue/concurrentqueue.h"

namespace ndnc {
class PendingInterest {
  public:
    PendingInterest() {
    }

    PendingInterest(std::shared_ptr<ndn::Interest> &&interest,
                    uint64_t pitToken, uint64_t timeoutCnt = 0) {
        this->pitToken = pitToken;
        this->timeoutCnt = timeoutCnt;
        this->lifetime = interest->getInterestLifetime().count();
        this->interest = getWireEncode(std::move(interest), pitToken);
    }

    void markAsExpressed() {
        this->expressedTimePoint = std::chrono::high_resolution_clock::now();
    }

    bool isExpired() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() -
                   this->expressedTimePoint)
                   .count() > lifetime;
    }

  public:
    uint64_t pitToken;
    uint64_t timeoutCnt;
    int64_t lifetime;
    ndn::Block interest;
    std::chrono::time_point<std::chrono::high_resolution_clock>
        expressedTimePoint;
};

typedef moodycamel::ConcurrentQueue<PendingInterest> RequestQueue;
typedef moodycamel::BlockingConcurrentQueue<std::shared_ptr<ndn::Data>>
    ResponseQueue;
}; // namespace ndnc

#endif // NDNC_PENDING_INTEREST_HPP
