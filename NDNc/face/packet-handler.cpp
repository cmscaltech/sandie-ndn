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
#include <iostream>

namespace ndnc {
PacketHandler::PacketHandler(Face &face) {
    face.addHandler(*this);
}

PacketHandler::~PacketHandler() {}

bool PacketHandler::enqueueInterestPacket(std::shared_ptr<ndn::Interest> &&,
                                          void *) {
    return true;
}

bool PacketHandler::enqueueInterests(
    std::vector<std::shared_ptr<ndn::Interest>> &&, size_t, void *) {
    return true;
}

void PacketHandler::dequeueDataPacket(std::shared_ptr<ndn::Data> &&,
                                      ndn::lp::PitToken &&) {}

bool PacketHandler::enqueueDataPacket(ndn::Data &&data,
                                      ndn::lp::PitToken &&pitToken) {
    return m_face != nullptr &&
           m_face->put(std::forward<ndn::Data>(data),
                       std::forward<ndn::lp::PitToken>(pitToken));
}

void PacketHandler::dequeueInterestPacket(std::shared_ptr<ndn::Interest> &&,
                                          ndn::lp::PitToken &&) {}

void PacketHandler::dequeueNackPacket(std::shared_ptr<ndn::lp::Nack> &&,
                                      ndn::lp::PitToken &&) {}
}; // namespace ndnc
