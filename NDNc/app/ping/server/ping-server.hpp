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

#include <ndn-cxx/encoding/block.hpp>

#include "face/packet-handler.hpp"

namespace ndnc {
namespace ping {
namespace server {

struct Options {
    /**
     * @brief Name prefix
     *
     */
    std::string prefix;

    /**
     * @brief Payload size
     *
     */
    size_t payloadSize = 128;
};

class Runner : public PacketHandler, public std::enable_shared_from_this<Runner> {
  public:
    explicit Runner(Face &face, Options options);

    struct Counters {
        uint32_t nRxInterests = 0;
        uint32_t nTxData = 0;
    };

    Counters readCounters();

  private:
    void processInterest(std::shared_ptr<ndn::Interest> &interest, ndn::lp::PitToken pitToken) final;

  private:
    ndn::Block m_payload;
    Options m_options;
    Counters m_counters;
};
}; // namespace server
}; // namespace ping
}; // namespace ndnc
