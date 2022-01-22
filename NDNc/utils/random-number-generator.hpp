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

#ifndef NDNC_UTILS_RAND_NUM_GEN_HPP
#define NDNC_UTILS_RAND_NUM_GEN_HPP

#include <mutex>
#include <random>

namespace ndnc {
template <typename T> class RandomNumberGenerator {
  public:
    RandomNumberGenerator()
        : m_engine{std::random_device()()},
          m_distribution(0, std::numeric_limits<T>::max()) {
    }

    T get() {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_distribution(m_engine);
    }

  private:
    std::mt19937_64 m_engine;
    std::uniform_int_distribution<T> m_distribution;
    std::mutex m_mtx;
};
} // namespace ndnc

#endif // NDNC_UTIL_RAND_NUM_GEN_HPP
