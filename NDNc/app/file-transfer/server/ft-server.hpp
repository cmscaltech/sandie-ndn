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

#ifndef NDNC_APP_FILE_TRANSFER_SERVER_FT_SERVER_HPP
#define NDNC_APP_FILE_TRANSFER_SERVER_FT_SERVER_HPP

#include "../common/ft-naming-scheme.hpp"
#include "face/packet-handler.hpp"
#include "lib/posix/file-metadata.hpp"
#include "lib/posix/file-rdr.hpp"

namespace ndnc::app::filetransfer {
struct ServerOptions {
    // GraphQL server address
    std::string gqlserver = "http://localhost:3030/";
    // Dataroom size
    size_t mtu = 9000;
    // Name prefix
    ndn::Name prefix = ndn::Name(NDNC_NAME_PREFIX_DEFAULT);

    // Segment size
    size_t segmentSize = 6600;
};
}; // namespace ndnc::app::filetransfer

namespace ndnc::app::filetransfer {
class Server : public PacketHandler {
  public:
    explicit Server(face::Face &face, ServerOptions options);
    ~Server();

  public:
    void onInterest(std::shared_ptr<ndn::Interest> &&interest,
                    ndn::lp::PitToken &&pitToken) final;

  private:
    std::shared_ptr<ndn::Data> getFileMetadata(const ndn::Name name);
    std::shared_ptr<ndn::Data> getFileContentData(const ndn::Name name);

  private:
    ServerOptions options_;
    ndn::Block payload_;
    ndn::SignatureInfo signatureInfo_;
};
}; // namespace ndnc::app::filetransfer

#endif // NDNC_APP_FILE_TRANSFER_SERVER_FT_SERVER_HPP
