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

#ifndef NDNC_CODECS_ENCODING_HPP
#define NDNC_CODECS_ENCODING_HPP

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>

#include <ndn-cxx/lp/nack.hpp>
#include <ndn-cxx/lp/packet.hpp>
#include <ndn-cxx/lp/pit-token.hpp>
#include <ndn-cxx/lp/tags.hpp>

namespace ndnc {
inline ndn::Block getWireEncode(std::shared_ptr<ndn::Interest> &&interest,
                                uint64_t pitTokenValue) {
    ndn::lp::Packet lpPacket(interest->wireEncode());

    // Add PIT TOKEN
    ndn::Buffer b(&pitTokenValue, sizeof(pitTokenValue));
    lpPacket.add<ndn::lp::PitTokenField>(
        ndn::lp::PitToken(std::make_pair(b.begin(), b.end())));

    return lpPacket.wireEncode();
}

inline std::shared_ptr<ndn::Interest> getWireDecode(ndn::Block wire) {
    ndn::lp::Packet lpPacket(wire);
    auto frag = lpPacket.get<ndn::lp::FragmentField>();

    return std::make_shared<ndn::Interest>(
        ndn::Block({frag.first, frag.second}));
}
} // namespace ndnc

namespace ndnc {
inline ndn::Block getWireEncode(std::shared_ptr<ndn::Data> &&data,
                                ndn::lp::PitToken &&pitToken) {
    ndn::lp::Packet lpPacket(data->wireEncode());
    lpPacket.add<ndn::lp::PitTokenField>(pitToken);

    return lpPacket.wireEncode();
}
} // namespace ndnc

namespace ndnc {
inline uint64_t getPITTokenValue(ndn::lp::PitToken &&pitToken) {
    return *((uint64_t *)pitToken.data());
}
} // namespace ndnc

#endif // NDNC_CODECS_ENCODING_HPP
