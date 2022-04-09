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

#include "pipeline/pipeline-interests-fixed.hpp"

namespace ndnc {
namespace ping {
namespace client {

struct Options {
    size_t mtu = 9000;                                 // Dataroom size
    std::string gqlserver = "http://172.17.0.2:3030/"; // GraphQL server address
    std::string name;                                  // Name prefix
    ndn::time::milliseconds lifetime =
        ndn::time::seconds{1}; // Interest lifetime
};

class Runner : public std::enable_shared_from_this<Runner> {
  public:
    struct Counters {
        uint32_t nTxInterests = 0;
        uint32_t nRxData = 0;
    };

    explicit Runner(face::Face &face, Options options);
    ~Runner();

    void run();
    void stop();
    bool canContinue();
    Counters readCounters();

  private:
    Options m_options;
    Counters m_counters;

    std::shared_ptr<PipelineInterests> m_pipeline;
    uint64_t m_sequence;
    bool m_stop;
};
}; // namespace client
}; // namespace ping
}; // namespace ndnc

#endif // NDNC_APP_PING_CLIENT_HPP
