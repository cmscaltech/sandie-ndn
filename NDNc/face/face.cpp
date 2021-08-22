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

#include <ndn-cxx/encoding/buffer.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/lp/nack.hpp>
#include <ndn-cxx/lp/packet.hpp>
#include <ndn-cxx/lp/tags.hpp>

#include "face.hpp"
#include "lp/pit-token.hpp"

namespace ndnc {
Face::Face() : m_transport(nullptr), m_valid(false), m_counters{} {
    m_client = std::make_unique<mgmt::Client>();
}

Face::~Face() {
    m_valid = false;

    if (m_client != nullptr) {
        if (!m_client->deleteFace()) {
#ifdef DEBUG
            ++m_counters.nErrors;
#endif // DEBUG
            std::cout << "WARN: Unable to delete face\n";
        }
    }

    m_transport = nullptr;
    m_packetHandler = nullptr;
}

bool Face::isValid() {
    return m_valid;
}

bool Face::advertise(const std::string prefix) {
    if (m_transport == nullptr || !m_transport->isUp()) {
#ifdef DEBUG
        ++m_counters.nErrors;
#endif // DEBUG
        return false;
    }

    if (m_client == nullptr || !this->isValid()) {
#ifdef DEBUG
        ++m_counters.nErrors;
#endif // DEBUG
        return false;
    }

    return m_client->advertiseOnFace(prefix);
}

Face::Counters Face::readCounters() {
    return this->m_counters;
}

void Face::loop() {
    m_transport->loop();
}

bool Face::addHandler(PacketHandler &h) {
    if (m_packetHandler == nullptr) {
#ifdef DEBUG
        ++m_counters.nErrors;
#endif // DEBUG
        return false;
    }

    m_packetHandler = &h;
    m_packetHandler->m_face = this;
    return true;
}

bool Face::send(const uint8_t *pkt, size_t pktLen) {
#ifdef DEBUG
    ++m_counters.nTxPackets;
    m_counters.nTxBytes += pktLen;
#endif // DEBUG

    if (pktLen > ndn::MAX_NDN_PACKET_SIZE) {
#ifdef DEBUG
        ++m_counters.nErrors;
#endif // DEBUG
        throw std::length_error("maximum NDN packet size breach");
        return false;
    }

    return m_transport->send(pkt, pktLen);
}

bool Face::expressInterest(const std::shared_ptr<const ndn::Interest> &interest,
                           const ndn::lp::PitToken &pitToken) {
    ndn::lp::Packet lpPacket(interest->wireEncode());
    lpPacket.add<ndn::lp::PitTokenField>(pitToken);

    auto wire = lpPacket.wireEncode();
    return this->send(wire.wire(), wire.size());
}

bool Face::putData(const ndn::Data &&data, const ndn::lp::PitToken &pitToken) {
    ndn::lp::Packet lpPacket(data.wireEncode());
    lpPacket.add<ndn::lp::PitTokenField>(pitToken);

    auto wire = lpPacket.wireEncode();
    return this->send(wire.wire(), wire.size());
}

void Face::transportRx(const uint8_t *pkt, size_t pktLen) {
#ifdef DEBUG
    ++m_counters.nRxPackets;
    m_counters.nRxBytes += pktLen;
#endif // DEBUG

    ndn::Block wire;
    bool isOk;

    std::tie(isOk, wire) = ndn::Block::fromBuffer(pkt, pktLen);
    if (!isOk) {
#ifdef DEBUG
        ++m_counters.nErrors;
#endif // DEBUG
        return;
    }

    ndn::lp::Packet lpPacket = ndn::lp::Packet(wire);

    ndn::Buffer::const_iterator begin, end;
    std::tie(begin, end) = lpPacket.get<ndn::lp::FragmentField>();

    ndn::Block netPacket(&*begin, std::distance(begin, end));
    switch (netPacket.type()) {
    case ndn::tlv::Interest: {
        auto interest = std::make_shared<const ndn::Interest>(netPacket);

        if (lpPacket.has<ndn::lp::NackField>()) {
            m_packetHandler->dequeueNackPacket(
                std::make_shared<const ndn::lp::Nack>(std::move(*interest)));
        } else {
            if (m_packetHandler != nullptr) {
                m_packetHandler->dequeueInterestPacket(
                    interest,
                    ndn::lp::PitToken(lpPacket.get<ndn::lp::PitTokenField>()));
            }
        }
        break;
    }

    case ndn::tlv::Data: {
        if (m_packetHandler != nullptr) {
            m_packetHandler->dequeueDataPacket(
                std::make_shared<const ndn::Data>(netPacket),
                ndn::lp::PitToken(lpPacket.get<ndn::lp::PitTokenField>()));
        }
        break;
    }

    default: {
        std::cout << "WARN: Unexpected packet type " << netPacket.type()
                  << "\n";
        break;
    }
    }
}

void Face::openMemif(int dataroom, std::string name, std::string gqlserver) {
    int id = 0;
    m_valid = m_client->openFace(id, dataroom, gqlserver);

    if (!m_valid) {
        std::cout << "WARN: Invalid face\n";
        return;
    }

#ifndef __APPLE__
    static Memif transport;
    if (!transport.begin(m_client->getSocketName().c_str(), id, name.c_str(),
                         dataroom)) {
        return;
    }

    this->m_transport = &transport;
#endif

    while (true) {
        if (m_transport->isUp()) {
            std::cout << "INFO: Transport is UP\n";
            m_transport->setRxCallback(transportRx, this);
            m_transport->setDisconnectCallback(onPeerDisconnect, this);
            break;
        }

        m_transport->loop();
    }
}

void Face::onPeerDisconnect() {
    std::cout << "WARN: Peer disconnected\n";
    this->m_valid = false;
}
}; // namespace ndnc
