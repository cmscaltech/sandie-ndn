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

#include <boost/lexical_cast.hpp>

#include <ndn-cxx/signature-info.hpp>
#include <ndn-cxx/util/sha256.hpp>

#include "logger/logger.hpp"
#include "ping-server.hpp"

namespace ndnc {
namespace ping {
Server::Server(face::Face &face, ServerOptions options)
    : PacketHandler(face), m_options{options}, m_counters{}, m_signatureInfo{} {

    auto buff = std::make_unique<ndn::Buffer>();
    buff->assign(m_options.payloadLength, 'p');
    m_payload = ndn::Block(ndn::tlv::Content, std::move(buff));

    m_signatureInfo.setSignatureType(ndn::tlv::DigestSha256);
}

Server::~Server() {
}

void Server::onInterest(std::shared_ptr<ndn::Interest> &&interest,
                        ndn::lp::PitToken &&pitToken) {
    ++m_counters.nRxInterests;

    LOG_INFO("%s %s", boost::lexical_cast<std::string>(pitToken).c_str(),
             interest->getName().toUri().c_str());

    auto data = std::make_shared<ndn::Data>(interest->getName());
    data->setContent(m_payload);
    data->setContentType(ndn::tlv::ContentType_Blob);
    data->setSignatureInfo(m_signatureInfo);
    data->setSignatureValue(std::make_shared<ndn::Buffer>());
    data->setFreshnessPeriod(ndn::time::seconds{2});

    if (face != nullptr &&
        !face->send(getWireEncode(std::move(data), std::move(pitToken)))) {
        LOG_WARN("unable to send Data packet on face");
        return;
    }

    ++m_counters.nTxData;
}

Server::Counters Server::getCounters() {
    return m_counters;
}
}; // namespace ping
}; // namespace ndnc
