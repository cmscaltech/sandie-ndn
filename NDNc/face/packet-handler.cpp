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

#include "packet-handler.hpp"

namespace ndnc {
PacketHandler::PacketHandler(Face &face) : m_pitGenerator{} {
    face.addHandler(*this);
}

PacketHandler::~PacketHandler() {
    if (m_face != nullptr) {
        m_face->removeHandler();
    }
}

void PacketHandler::loop() {}

uint64_t
PacketHandler::expressInterest(std::shared_ptr<const ndn::Interest> interest) {
    if (m_face != nullptr &&
        m_face->expressInterest(interest, m_pitGenerator.getNext())) {
        return m_pitGenerator.getSequenceValue();
    }

    throw std::runtime_error("unable to express Interest on face");
    return 0;
}

bool PacketHandler::putData(std::shared_ptr<ndn::Data> &data,
                            ndn::lp::PitToken pitToken) {
    return m_face != nullptr && m_face->putData(data, pitToken);
}

void PacketHandler::processInterest(std::shared_ptr<ndn::Interest> &,
                                    ndn::lp::PitToken) {}

void PacketHandler::processData(std::shared_ptr<ndn::Data> &, uint64_t) {}

void PacketHandler::processNack(std::shared_ptr<ndn::lp::Nack> &) {}

void PacketHandler::onData(std::shared_ptr<ndn::Data> &data,
                           ndn::lp::PitToken pitToken) {
    this->processData(data, lp::PitTokenGenerator::getPitValue(pitToken));
}
}; // namespace ndnc
