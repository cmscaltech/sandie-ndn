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

#include "face.hpp"
#include "logger/logger.hpp"

namespace ndnc {
Face::Face() : m_transport(nullptr), m_packetHandler(nullptr), m_valid(false) {
    m_client = std::make_unique<mgmt::Client>();
    m_counters = std::make_shared<Counters>();
}

Face::~Face() {
    m_valid = false;

    if (m_client != nullptr) {
        m_client->deleteFace();
    }

    m_transport = nullptr;
    m_packetHandler = nullptr;
}

bool Face::addPacketHandler(PacketHandler &h) {
    m_packetHandler = &h;

    if (m_packetHandler == nullptr) {
        LOG_ERROR("null packet handler");
        m_valid = false;
        return false;
    }

    m_packetHandler->face = this;
    return true;
}

bool Face::openMemif(int dataroom, std::string gqlserver, std::string name) {
    m_valid = m_client->createFace(0, dataroom, gqlserver);

    if (!isValid()) {
        LOG_ERROR("unable to open memif face");
        return false;
    }

#ifndef __APPLE__
    static Memif transport;
    if (!transport.init(m_client->getSocketName().c_str(), 0, name.c_str(),
                        dataroom)) {
        LOG_ERROR("unable to initialize memif face");
        return false;
    }

    this->m_transport = &transport;
#endif // __APPLE__

    while (true) {
        if (m_transport->isUp()) {
            m_transport->setRxCallback(receive, this);
            m_transport->setDisconnectCallback(disconnect, this);
            break;
        }

        m_transport->loop();
    }

    return true;
}

bool Face::advertise(const std::string prefix) {
    if (m_transport == nullptr || !m_transport->isUp()) {
        return false;
    }

    if (m_client == nullptr || !this->isValid()) {
        return false;
    }

    return m_client->insertFibEntry(prefix);
}

bool Face::isValid() {
    return m_valid;
}

void Face::loop() {
    m_transport->loop();
}

std::shared_ptr<Face::Counters> Face::readCounters() {
    return this->m_counters;
}

bool Face::send(ndn::Block &&wire) {
    if (!m_transport->send(std::move(wire))) {
#ifndef NDEBUG
        ++m_counters->nErrors;
#endif // NDEBUG
        return false;
    }

#ifndef NDEBUG
    ++m_counters->nTxPackets;
    m_counters->nTxBytes += wire.size();
#endif // NDEBUG
    return true;
}

bool Face::send(std::vector<ndn::Block> &&wires) {
    if (!m_transport->send(std::move(wires))) {
#ifndef NDEBUG
        ++m_counters->nErrors;
#endif // NDEBUG
        return false;
    }

#ifndef NDEBUG
    m_counters->nTxPackets += wires.size();
    for (auto wire = wires.begin(); wire != wires.end(); ++wire)
        m_counters->nTxBytes += wire->size();
#endif // NDEBUG
    return true;
}

void Face::receive(const uint8_t *pkt, size_t pktLen) {
#ifndef NDEBUG
    ++m_counters->nRxPackets;
    m_counters->nRxBytes += pktLen;
#endif // NDEBUG

    ndn::Block wire;
    bool isOk;

    std::tie(isOk, wire) = ndn::Block::fromBuffer(pkt, pktLen);
    if (!isOk) {
#ifndef NDEBUG
        ++m_counters->nErrors;
#endif // NDEBUG
        return;
    }

    ndn::lp::Packet lpPacket = ndn::lp::Packet(wire);

    ndn::Buffer::const_iterator begin, end;
    std::tie(begin, end) = lpPacket.get<ndn::lp::FragmentField>();

    ndn::Block netPacket(&*begin, std::distance(begin, end));
    switch (netPacket.type()) {
    case ndn::tlv::Interest: {
        auto interest = std::make_shared<ndn::Interest>(netPacket);
        auto pitToken =
            ndn::lp::PitToken(lpPacket.get<ndn::lp::PitTokenField>());

        if (lpPacket.has<ndn::lp::NackField>()) {
            m_packetHandler->onNack(
                std::make_shared<ndn::lp::Nack>(std::move(*interest)),
                std::move(pitToken));
        } else {
            if (m_packetHandler != nullptr) {
                m_packetHandler->onInterest(std::move(interest),
                                            std::move(pitToken));
            }
        }
        break;
    }

    case ndn::tlv::Data: {
        if (m_packetHandler != nullptr) {
            m_packetHandler->onData(
                std::make_shared<ndn::Data>(netPacket),
                ndn::lp::PitToken(lpPacket.get<ndn::lp::PitTokenField>()));
        }
        break;
    }

    default: {
        LOG_WARN("unexpected packet type=%i", netPacket.type());
        break;
    }
    }
}

void Face::disconnect() {
    LOG_FATAL("peer disconnected");
    this->m_valid = false;
}
}; // namespace ndnc
