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

#ifndef NDNC_FACE_HPP
#define NDNC_FACE_HPP

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
    /**
     * @brief Face counters - enabled for debug builds only
     *
     */
    struct Counters {
        uint64_t nTxPackets = 0;
        uint64_t nRxPackets = 0;
        uint64_t nTxBytes = 0;
        uint64_t nRxBytes = 0;
        uint16_t nErrors = 0;
    };

  public:
    Face();
    ~Face();

    bool addHandler(PacketHandler &h);
    bool removeHandler();

    bool isValid();
    void loop();
    bool advertise(const std::string prefix);

    bool expressInterest(const std::shared_ptr<const ndn::Interest> &interest,
                         const ndn::lp::PitToken &pitToken);
    bool putData(const ndn::Data &&data, const ndn::lp::PitToken &pitToken);

    Counters readCounters();

  private:
    /**
     * @brief Open memif face between application and local NDN-DPDK forwarder
     *
     */
    void openMemif();

    /**
     * @brief Handle peer disconnect events by gracefully closing memif face
     *
     */
    void onPeerDisconnect();
    static void onPeerDisconnect(void *self) {
        reinterpret_cast<Face *>(self)->onPeerDisconnect();
    }

    /**
     * @brief Handle memif interupts - packets arrival
     *
     * @param pkt Received packet over memif face
     * @param pktLen Size of received packet
     */
    void transportRx(const uint8_t *pkt, size_t pktLen);
    static void transportRx(void *self, const uint8_t *pkt, size_t pktLen) {
        reinterpret_cast<Face *>(self)->transportRx(pkt, pktLen);
    }

    /**
     * @brief Send packet over memif face
     *
     * @param pkt Packet to send
     * @param pktLen Size of packet to send
     * @return true Packet was successfully sent
     * @return false Packet could not be sent
     */
    bool send(const uint8_t *pkt, size_t pktLen);

  private:
    std::unique_ptr<graphql::Client> m_client;
    Transport *m_transport;
    PacketHandler *m_packetHandler;

    bool m_valid;
    Counters m_counters;
};
}; // namespace ndnc

#endif // NDNC_FACE_HPP
