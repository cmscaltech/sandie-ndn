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
Face::Face()
    : m_transport{nullptr}, m_packetHandler{nullptr}, m_hasError{false} {
    m_gqlClient = std::make_shared<mgmt::Client>();
}

Face::~Face() {
    if (m_gqlClient != nullptr) {
        m_gqlClient->deleteFace();
    }

    m_packetHandler = nullptr;
}

bool Face::connect(int dataroom, std::string gqlserver, std::string appName) {
    if (!m_gqlClient->createFace(0, dataroom, gqlserver)) {
        LOG_ERROR("unable to create memif");
        return false;
    }

#if (!defined(__APPLE__) && !defined(__MACH__))
    try {
        m_transport = std::make_shared<transport::Memif>(
            dataroom, m_gqlClient->getSocketPath().c_str(), appName.c_str());
    } catch (const std::exception &e) {
        LOG_FATAL("%s", e.what());
        return false;
    }
#endif

    if (!m_transport->connect()) {
        return false;
    }

    m_transport->setOnDisconnectCallback(
        [](void *self) { reinterpret_cast<Face *>(self)->disconnect(); }, this);

    m_transport->setOnReceiveCallback(
        [](void *self, const ndn::Block &&pkt) {
            reinterpret_cast<Face *>(self)->receive(std::move(pkt));
        },
        this);

    return true;
}

bool Face::isConnected() {
    return m_transport != nullptr && m_transport->isConnected() && !m_hasError;
}

void Face::disconnect() {
    this->onDisconnect();
}

bool Face::loop() {
    return m_transport->loop();
}

bool Face::addPacketHandler(PacketHandler &h) {
    m_packetHandler = &h;

    if (m_packetHandler == nullptr) {
        LOG_FATAL("add null packet handler");
        m_hasError = true;

        return false;
    }

    m_packetHandler->face = this;
    return true;
}

void Face::addOnDisconnectHandler(std::function<void()> cb) {
    this->onDisconnect = cb;
}

bool Face::advertise(const std::string prefix) {
    if (!isConnected()) {
        return false;
    }

    return m_gqlClient->insertFibEntry(prefix);
}

int Face::send(const ndn::Block pkt) {
    return m_transport->send(pkt);
}

int Face::send(const std::vector<ndn::Block> *pkts, uint16_t n) {
    return m_transport->send(pkts, n);
}

void Face::receive(const ndn::Block &&pkt) {
    ndn::lp::Packet lpPacket = ndn::lp::Packet(pkt);
    auto frag = lpPacket.get<ndn::lp::FragmentField>();

    ndn::Block netPacket({frag.first, frag.second});
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
        LOG_WARN("received unexpected packet type=%i", netPacket.type());
        return;
    }
    }
}
}; // namespace face
}; // namespace ndnc
