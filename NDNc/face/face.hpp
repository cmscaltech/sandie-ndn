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

#ifndef NDNC_FACE_HH
#define NDNC_FACE_HH

#include <memory>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/lp/pit-token.hpp>

#include "graphql/client.hpp"

#ifndef __APPLE__
#include "memif.hpp"
#else
#include "transport.hpp"
#endif

namespace ndnc {
class PacketHandler;
};
#include "packet-handler.hpp"

namespace ndnc {

class Face {
  public:
    Face();
    ~Face();

    bool addHandler(PacketHandler &h);
    bool removeHandler();

    bool isValid();
    void loop();
    bool advertise(std::string prefix);

    bool expressInterest(std::shared_ptr<const ndn::Interest> interest);
    bool putData(std::shared_ptr<ndn::Data> &data, ndn::lp::PitToken pitToken);

  private:
    void openMemif();
    bool send(const uint8_t *pkt, size_t pktLen); // TODO: Throw error

    void transportRx(const uint8_t *pkt, size_t pktLen);
    void onPeerDisconnect();

  private:
    static void transportRx(void *self, const uint8_t *pkt, size_t pktLen) {
        reinterpret_cast<Face *>(self)->transportRx(pkt, pktLen);
    }

    static void onPeerDisconnect(void *self) { reinterpret_cast<Face *>(self)->onPeerDisconnect(); }

  private:
    std::unique_ptr<graphql::Client> m_client;
    Transport *m_transport;
    PacketHandler *m_packetHandler;

    bool m_valid;
};
}; // namespace ndnc

#endif // NDNC_FACE_HH
