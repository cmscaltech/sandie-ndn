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
#include <ndn-cxx/encoding/buffer.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/lp/nack.hpp>
#include <ndn-cxx/lp/packet.hpp>
#include <ndn-cxx/lp/pit-token.hpp>

#include "face.hpp"

namespace ndnc {
Face::Face() : m_transport(NULL) {
    m_client = std::make_unique<graphql::Client>();
    m_valid  = m_client->openFace();
    this->openMemif();
}

Face::~Face() {
    if (m_client != nullptr) {
        m_client->deleteFace();
    }
}

bool Face::isValid() {
    return m_valid;
}

bool Face::advertise(std::string prefix) {
    if (NULL == m_transport || !m_transport->isUp()) {
        return false;
    }

    if (!m_valid || m_client == nullptr) {
        return false;
    }

    return m_client->advertiseOnFace(prefix);
}

void Face::loop() {
    m_transport->loop();
}

bool Face::addHandler(PacketHandler& h) {
    m_packetHandler = &h;
    return true;
}

bool Face::removeHandler() {
    m_packetHandler = NULL;
    return true;
}

void Face::transportRx(const uint8_t* pkt, size_t pktLen) {
    ndn::Block wire;
    bool isOk;

    std::tie(isOk, wire) = ndn::Block::fromBuffer(pkt, pktLen);
    if (!isOk) {
        return;
    }

    ndn::lp::Packet lpPacket;
    try {
        lpPacket = ndn::lp::Packet(wire);
    } catch (...) {
        return;
    }

    ndn::Buffer::const_iterator begin, end;
    std::tie(begin, end) = lpPacket.get<ndn::lp::FragmentField>();

    ndn::Block netPacket(&*begin, std::distance(begin, end));
    switch (netPacket.type()) {
    case ndn::tlv::Interest: {
        auto interest = std::make_shared<ndn::Interest>(netPacket);

        if (lpPacket.has<ndn::lp::NackField>()) {
            auto nack = std::make_shared<ndn::lp::Nack>(std::move(*interest));
            std::cout << "Recieved NACK\n";
        } else {
            if (NULL != m_packetHandler) {
                m_packetHandler->processInterest(interest);
            }
            // std::cout << "[" << boost::lexical_cast<std::string>(ndn::lp::PitToken(lpPacket.get<ndn::lp::PitTokenField>())) << "] ";
            // std::cout << "Interest Name: " << interest->getName().toUri() << "\n";
        }
        break;
    }

    case ndn::tlv::Data:
    default: {
        std::cout << "WARNING: Unexpected packet type " << netPacket.type() << "\n";
        break;
    }
    }
}

void Face::openMemif() {
    static transport::Memif transport;
    if (!transport.begin(m_client->getSocketName().c_str(), 0)) {
        return;
    }

    this->m_transport = &transport;

    while (true) {
        if (m_transport->isUp()) {
            std::cout << "INFO: Transport is UP\n";
            m_transport->setRxCallback(transportRx, this);
            break;
        }

        m_transport->loop();
    }
}
}; // namespace ndnc
