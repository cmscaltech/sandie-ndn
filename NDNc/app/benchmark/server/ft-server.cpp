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

#include <math.h>

#include "ft-server.hpp"
#include "logger/logger.hpp"

namespace ndnc {
namespace benchmark {
namespace ft {
Runner::Runner(Face &face, ServerOptions options)
    : PacketHandler(face), m_options{options}, m_signatureInfo{} {

    auto buff = std::make_unique<ndn::Buffer>();
    buff->assign(m_options.segmentSize, 'p');
    m_payload = ndn::Block(ndn::tlv::Content, std::move(buff));

    m_signatureInfo.setSignatureType(ndn::tlv::DigestSha256);
}

Runner::~Runner() {
}

void Runner::onInterest(std::shared_ptr<ndn::Interest> &&interest,
                        ndn::lp::PitToken &&pitToken) {

    auto name = interest->getName();
    auto data =
        isMetadataName(name) ? getFileInfoData(name) : getFileContentData(name);

    data->setSignatureInfo(m_signatureInfo);
    data->setSignatureValue(std::make_shared<ndn::Buffer>());

    if (face != nullptr &&
        !face->send(getWireEncode(std::move(data), std::move(pitToken)))) {
        LOG_WARN("unable to send Data packet");
    }
}

std::shared_ptr<ndn::Data> Runner::getFileInfoData(const ndn::Name name) {
    LOG_INFO("received meta Interest %s", name.toUri().c_str());

    auto data = std::make_shared<ndn::Data>(name);
    FileMetadata metadata{m_options.segmentSize};

    if (metadata.prepare(getFilePathFromMetadataName(name))) {
        data->setContent(metadata.encode());
        data->setContentType(ndn::tlv::ContentType_Blob);
    } else {
        data->setContent(nullptr, 0);
        data->setContentType(ndn::tlv::ContentType_Nack);
    }

    data->setFreshnessPeriod(ndn::time::milliseconds{2});
    return data;
}

std::shared_ptr<ndn::Data> Runner::getFileContentData(const ndn::Name name) {
    auto data = std::make_shared<ndn::Data>(name);

    data->setContent(m_payload);
    data->setContentType(ndn::tlv::ContentType_Blob);
    return data;
}
}; // namespace ft
}; // namespace benchmark
}; // namespace ndnc
