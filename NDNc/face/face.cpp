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
namespace face {
Face::Face() : m_transport(nullptr), m_packetHandler(nullptr), m_valid(false) {
    m_client = std::make_unique<mgmt::Client>();
    m_counters = std::make_shared<Counters>();
}

Face::~Face() {
    m_valid = false;

    if (m_client != nullptr) {
        m_client->deleteFace();
    }

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

bool Face::advertiseNamePrefix(const std::string prefix, void *ctx) {
    if (m_transport == nullptr || !m_transport->isConnected(ctx)) {
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

bool Face::send(ndn::Block pkt, void *ctx) {
    if (m_transport->send(pkt, ctx) == 0) {
#ifndef NDEBUG
        ++m_counters->nErrors;
#endif // NDEBUG
        return false;
    }

#ifndef NDEBUG
    m_counters->nTxPackets += 1;
    m_counters->nTxBytes += pkt.size();
#endif // NDEBUG
    return true;
}

bool Face::send(std::vector<ndn::Block> &&pkts, uint16_t n, void *ctx) {
    auto tx = m_transport->send(std::move(pkts), n, ctx);

    if (tx == 0) {
#ifndef NDEBUG
        ++m_counters->nErrors;
#endif // NDEBUG
        return false;
    }

#ifndef NDEBUG
    m_counters->nTxPackets += tx;
    for (auto i = 0; i < tx; ++i) {
        m_counters->nTxBytes += pkts[i].size();
    }
#endif // NDEBUG
    return true;
}

bool Face::openMemif(int dataroom, std::string gqlserver, std::string appName) {
    uint32_t id = 0;
    m_valid = m_client->createFace(id, dataroom, gqlserver);

    if (!isValid()) {
        LOG_ERROR("unable to open memif face");
        return false;
    }

#if (!defined(__APPLE__) && !defined(__MACH__))
    try {
        this->m_transport = std::make_shared<transport::Memif>(
            dataroom, m_client->getSocketPath().c_str(), appName.c_str());
    } catch (const std::exception &e) {
        m_valid = false;
        LOG_FATAL(e.what());
        return false;
    }
#endif

    m_transport->setContext(this);

    m_transport->setOnReceiveCallback(
        [](void *self, const uint8_t *pkt, size_t pktLen) {
            reinterpret_cast<Face *>(self)->onTransportReceive(pkt, pktLen);
        });

    return true;
}

void Face::onTransportReceive(const uint8_t *pkt, size_t pktLen) {
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
            m_packetHandler->onInterest(std::move(interest),
                                        std::move(pitToken));
        }
        return;
    }

    case ndn::tlv::Data: {
        m_packetHandler->onData(
            std::make_shared<ndn::Data>(netPacket),
            ndn::lp::PitToken(lpPacket.get<ndn::lp::PitTokenField>()));

        return;
    }

    default: {
        LOG_WARN("unexpected packet type=%i", netPacket.type());
        return;
    }
    }
}
}; // namespace face
}; // namespace ndnc
