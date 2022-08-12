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

#ifndef NDNC_APP_PING_SERVER_PING_SERVER_HPP
#define NDNC_APP_PING_SERVER_PING_SERVER_HPP

#include "face/packet-handler.hpp"

namespace ndnc {
namespace ping {

struct ServerOptions {
    size_t mtu = 9000;                                // Dataroom size
    std::string gqlserver = "http://localhost:3030/"; // GraphQL server address
    std::string name;                                 // Name prefix
    size_t payloadLength = 0;                         // Payload length
};

class Server : public PacketHandler,
               public std::enable_shared_from_this<Server> {
  public:
    struct Counters {
        uint32_t nRxInterests = 0;
        uint32_t nTxData = 0;
    };

    explicit Server(face::Face &face, ServerOptions options);
    ~Server();

    Counters getCounters();

  private:
    void onInterest(std::shared_ptr<ndn::Interest> &&interest,
                    ndn::lp::PitToken &&pitToken) final;

  private:
    ServerOptions m_options;
    Counters m_counters;

    ndn::Block m_payload;
    ndn::SignatureInfo m_signatureInfo;
};
}; // namespace ping
}; // namespace ndnc

#endif // NDNC_APP_PING_SERVER_PING_SERVER_HPP
