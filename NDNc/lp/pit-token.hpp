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

#ifndef NDNC_LP_PIT_TOKEN_HPP
#define NDNC_LP_PIT_TOKEN_HPP

#include <random>

#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/lp/fields.hpp>
#include <ndn-cxx/lp/pit-token.hpp>
#include <ndn-cxx/lp/tlv.hpp>

namespace ndnc {
namespace lp {
class PitTokenGenerator {
  public:
    PitTokenGenerator() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dist(
            std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint64_t>::max());

        m_sequence = dist(gen);
    }

    auto getToken() {
        auto block = ndn::encoding::makeNonNegativeIntegerBlock(
            ndn::lp::tlv::PitToken, ++m_sequence);

        return ndn::lp::PitToken(
            std::make_pair(block.value_begin(), block.value_end()));
    }

    auto getSequenceValue() { return m_sequence; }

  public:
    inline static uint64_t getTokenValue(const uint8_t *data) {
        return (((uint64_t)(data[7]) << 0) + ((uint64_t)(data[6]) << 8) +
                ((uint64_t)(data[5]) << 16) + ((uint64_t)(data[4]) << 24) +
                ((uint64_t)(data[3]) << 32) + ((uint64_t)(data[2]) << 40) +
                ((uint64_t)(data[1]) << 48) + ((uint64_t)(data[0]) << 56));
    }

  private:
    uint64_t m_sequence;
};
}; // namespace lp
}; // namespace ndnc

#endif // NDNC_LP_PIT_TOKEN_HPP
