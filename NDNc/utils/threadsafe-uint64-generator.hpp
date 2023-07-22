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

#ifndef NDNC_UTILS_THREAD_SAFE_UINT64_GENERATOR_HPP
#define NDNC_UTILS_THREAD_SAFE_UINT64_GENERATOR_HPP

#include <cstdint>
#include <mutex>
#include <random>

namespace ndnc {
class ThreadSafeUInt64Generator {
  public:
    ThreadSafeUInt64Generator()
        : generator_(std::random_device{}()),
          distribution_(0, std::numeric_limits<uint64_t>::max()) {
    }

    uint64_t generate() {
        std::lock_guard<std::mutex> lock(mutex_);
        return distribution_(generator_);
    }

  private:
    std::mutex mutex_;
    std::mt19937_64 generator_;
    std::uniform_int_distribution<uint64_t> distribution_;
};
}; // namespace ndnc

#endif // NDNC_UTILS_THREAD_SAFE_UINT64_GENERATOR_HPP
