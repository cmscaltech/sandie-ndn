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

#include "packet-handler.hpp"

namespace ndnc {
PacketHandler::PacketHandler(Face &face)
    : m_pitTokenGen(new ndnc::lp::PitTokenGenerator), m_pitToInterestLifetime{},
      m_interestLifetimeToPit{} {
    face.addHandler(*this);
}

PacketHandler::~PacketHandler() {
    if (m_face != nullptr) {
        m_face->removeHandler();
    }
}

bool PacketHandler::removePendingInterestEntry(uint64_t pitToken) {
    if (m_pitToInterestLifetime.count(pitToken) == 0) {
        std::cout << "WARN: Received unexpected Data\n";
        return false;
    }

    m_interestLifetimeToPit.erase(m_pitToInterestLifetime[pitToken]);
    m_pitToInterestLifetime.erase(pitToken);
    return true;
}

void PacketHandler::doLoop() {
    if (!m_interestLifetimeToPit.empty()) {
        auto entry = *m_interestLifetimeToPit.begin();

        if (ndn::time::system_clock::now() > entry.first) {
            if (this->removePendingInterestEntry(entry.second)) {
                this->onTimeout(entry.second);
            }
        }
    }

    this->loop();
}

void PacketHandler::loop() {}

uint64_t
PacketHandler::expressInterest(std::shared_ptr<const ndn::Interest> interest) {
    if (m_face != nullptr &&
        m_face->expressInterest(interest, m_pitTokenGen->getToken())) {

        uint64_t key = m_pitTokenGen->getSequenceValue();
        auto value =
            ndn::time::system_clock::now() + interest->getInterestLifetime();

        m_pitToInterestLifetime[key] = value;
        m_interestLifetimeToPit[value] = key;

        return key;
    }

    throw std::runtime_error("unable to express Interest on face");
    return 0;
}

bool PacketHandler::putData(std::shared_ptr<ndn::Data> &data,
                            const ndn::lp::PitToken &pitToken) {
    return m_face != nullptr && m_face->putData(data, pitToken);
}

void PacketHandler::onData(std::shared_ptr<ndn::Data> &data,
                           ndn::lp::PitToken pitToken) {
    auto pit = lp::PitTokenGenerator::getTokenValue(pitToken.data());

    if (removePendingInterestEntry(pit)) {
        this->processData(data, pit);
    }
}

void PacketHandler::processInterest(std::shared_ptr<ndn::Interest> &,
                                    const ndn::lp::PitToken &) {}

void PacketHandler::processData(std::shared_ptr<ndn::Data> &, uint64_t) {}

void PacketHandler::processNack(std::shared_ptr<ndn::lp::Nack> &) {}

void PacketHandler::onTimeout(uint64_t pitToken) {}
}; // namespace ndnc
