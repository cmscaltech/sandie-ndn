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

#include <iostream>

#include <boost/lexical_cast.hpp>

#include <ndn-cxx/signature-info.hpp>
#include <ndn-cxx/util/sha256.hpp>

#include "file-transfer-server.hpp"

namespace ndnc {
namespace benchmark {
namespace fileTransferServer {
Runner::Runner(Face &face, Options options)
    : PacketHandler(face), m_options{options}, m_signatureInfo{} {
    m_signatureInfo.setSignatureType(ndn::tlv::DigestSha256);

    auto buff = std::make_unique<ndn::Buffer>();
    buff->assign(m_options.payloadSize, 't');
    m_payload = ndn::Block(ndn::tlv::Content, std::move(buff));
}

void Runner::dequeueInterestPacket(
    const std::shared_ptr<const ndn::Interest> &interest,
    const ndn::lp::PitToken &pitToken) {
    std::cout << ndn::time::toString(ndn::time::system_clock::now()) << " "
              << boost::lexical_cast<std::string>(pitToken) << " "
              << interest->getName() << "\n";

    auto data = ndn::Data(interest->getName());
    data.setContent(m_payload);
    data.setContentType(ndn::tlv::ContentType_Blob);
    data.setSignatureInfo(m_signatureInfo);
    data.setSignatureValue(std::make_shared<ndn::Buffer>());

    if (!enqueueDataPacket(std::move(data), pitToken)) {
        std::cout << "ERROR: Unable to put Data\n";
    }
}
}; // namespace fileTransferServer
}; // namespace benchmark
}; // namespace ndnc