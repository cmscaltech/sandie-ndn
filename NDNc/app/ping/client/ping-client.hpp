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

#ifndef NDNC_APP_PING_CLIENT_HPP
#define NDNC_APP_PING_CLIENT_HPP

#include <unordered_map>

#include "face/packet-handler.hpp"

namespace ndnc {
namespace ping {
namespace client {

struct Options {
    /**
     * @brief Name prefix
     *
     */
    std::string prefix;

    /**
     * @brief Interval between Interests
     *
     */
    ndn::time::milliseconds interval = ndn::time::milliseconds{100};

    /**
     * @brief Interest lifetime
     *
     */
    ndn::time::milliseconds lifetime = ndn::time::seconds{1};
};

class Runner : public PacketHandler,
               public std::enable_shared_from_this<Runner> {
  public:
    explicit Runner(Face &face, Options options);

    struct Counters {
        uint32_t nTxInterests = 0;
        uint32_t nRxNacks = 0;
        uint32_t nRxData = 0;
        uint32_t nTimeout = 0;
    };

    Counters readCounters();

  private:
    void loop() final;

    bool sendInterest();
    void processData(const std::shared_ptr<const ndn::Data> &data,
                     uint64_t pitToken) final;
    void processNack(const std::shared_ptr<const ndn::lp::Nack> &nack) final;
    void onTimeout(uint64_t pitToken) final;

  private:
    Options m_options;
    Counters m_counters;

    std::unordered_map<uint64_t, ndn::time::system_clock::time_point>
        m_pendingInterests;

    uint64_t m_sequence;
    ndn::time::system_clock::time_point m_next;
};
}; // namespace client
}; // namespace ping
}; // namespace ndnc

#endif // NDNC_APP_PING_CLIENT_HPP
