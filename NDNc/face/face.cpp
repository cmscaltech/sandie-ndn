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
Face::Face() : m_transport(NULL), m_valid(false), m_counters{} {
    m_client = std::make_unique<graphql::Client>();
#ifndef __APPLE__
    m_valid = m_client->openFace();
    this->openMemif();
#endif
}

Face::~Face() {
    if (m_client != nullptr) {
        if (!m_client->deleteFace()) {
#ifdef DEBUG
            ++m_counters.nErrors;
#endif // DEBUG
            std::cout << "WARN: Unable to delete face\n";
        }
    }

    m_transport = NULL;
}

bool Face::isValid() {
    return m_valid;
}

bool Face::advertise(const std::string prefix) {
    if (NULL == m_transport || !m_transport->isUp()) {
#ifdef DEBUG
        ++m_counters.nErrors;
#endif // DEBUG
        return false;
    }

    if (!isValid() || m_client == nullptr) {
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

void Face::printCounters() {
    std::cout << "nTxPackets: " << m_counters.nTxPackets
              << " nTxBytes: " << m_counters.nTxBytes
              << " nRxPackets: " << m_counters.nRxPackets
              << " nRxBytes: " << m_counters.nRxBytes
              << " nErros: " << m_counters.nErrors << "\n";
}

void Face::loop() {
    m_transport->loop();

    if (m_packetHandler != nullptr) {
        m_packetHandler->doLoop();
    }
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

bool Face::removeHandler() {
    m_packetHandler = NULL;
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

    ndn::lp::Packet lpPacket;
    try {
        lpPacket = ndn::lp::Packet(wire);
    } catch (...) {
#ifdef DEBUG
        ++m_counters.nErrors;
#endif // DEBUG
        return;
    }

    ndn::Buffer::const_iterator begin, end;
    std::tie(begin, end) = lpPacket.get<ndn::lp::FragmentField>();

    ndn::Block netPacket(&*begin, std::distance(begin, end));
    switch (netPacket.type()) {
    case ndn::tlv::Interest: {
        auto interest = std::make_shared<const ndn::Interest>(netPacket);

        if (lpPacket.has<ndn::lp::NackField>()) {
            m_packetHandler->processNack(
                std::make_shared<const ndn::lp::Nack>(std::move(*interest)));
        } else {
            if (NULL != m_packetHandler) {
                m_packetHandler->processInterest(
                    interest,
                    ndn::lp::PitToken(lpPacket.get<ndn::lp::PitTokenField>()));
            }
        }
        break;
    }

    case ndn::tlv::Data: {
        if (NULL != m_packetHandler) {
            m_packetHandler->onData(
                std::make_shared<const ndn::Data>(netPacket),
                ndn::lp::PitToken(lpPacket.get<ndn::lp::PitTokenField>()));
        }
        break;
    }

    default: {
        std::cout << "WARNING: Unexpected packet type " << netPacket.type()
                  << "\n";
        break;
    }
    }
}

void Face::openMemif() {
    if (!isValid()) {
        std::cout << "WARN: Invalid face\n";
        return;
    }

#ifndef __APPLE__
    static Memif transport;
    if (!transport.begin(m_client->getSocketName().c_str(), 1)) {
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
