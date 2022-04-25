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

#ifndef NDNC_APP_BENCHMARK_FT_SERVER_HPP
#define NDNC_APP_BENCHMARK_FT_SERVER_HPP

#include "../common/file-metadata.hpp"
#include "face/packet-handler.hpp"

namespace ndnc {
namespace benchmark {
namespace ft {

struct ServerOptions {
    size_t segmentSize = 6600;
    size_t mtu = 9000;                                // Dataroom size
    std::string gqlserver = "http://localhost:3030/"; // GraphQL server address
};

class Runner : public PacketHandler {
  public:
    explicit Runner(face::Face &face, ServerOptions options);
    ~Runner();

  public:
    void onInterest(std::shared_ptr<ndn::Interest> &&interest,
                    ndn::lp::PitToken &&pitToken) final;

  private:
    std::shared_ptr<ndn::Data> getFileMetadata(const ndn::Name name);
    std::shared_ptr<ndn::Data> getFileContentData(const ndn::Name name);

  private:
    ServerOptions m_options;
    ndn::Block m_payload;
    ndn::SignatureInfo m_signatureInfo;
};
}; // namespace ft
}; // namespace benchmark
}; // namespace ndnc

#endif // NDNC_APP_BENCHMARK_FT_SERVER_HPP
