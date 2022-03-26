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

#include <atomic>
#include <memory>
#if (!defined(__APPLE__) && !defined(__MACH__))
#include "memif.hpp"
#else
#include "transport.hpp"
#endif
#include "mgmt/client.hpp"

namespace ndnc {
class PacketHandler;
};
#include "packet-handler.hpp"

namespace ndnc {
namespace face {
class Face {
  public:
    // Face counters - enabled for debug builds only
    struct Counters {
        std::atomic<uint64_t> nTxPackets = 0;
        std::atomic<uint64_t> nRxPackets = 0;
        std::atomic<uint64_t> nTxBytes = 0;
        std::atomic<uint64_t> nRxBytes = 0;
        std::atomic<uint16_t> nErrors = 0;
    };

  public:
    Face();
    ~Face();

    bool addPacketHandler(PacketHandler &h);
    bool openMemif(int dataroom, std::string gqlserver, std::string name);
    bool isValid();
    void loop();

    bool advertiseNamePrefix(const std::string prefix, void *);

    bool send(ndn::Block, void *);
    bool send(std::vector<ndn::Block> &&, uint16_t, void *);

    std::shared_ptr<Counters> readCounters();

  private:
    /**
     * @brief Handle peer interupts - packets arrival
     *
     * @param pkt Received packet over memif face
     * @param pktLen Size of received packet
     */
    void onTransportReceive(const uint8_t *pkt, size_t pktLen);

  private:
    std::unique_ptr<mgmt::Client> m_client;
    std::shared_ptr<Counters> m_counters;

    std::shared_ptr<transport::Transport> m_transport;
    PacketHandler *m_packetHandler;

    bool m_valid;
};
}; // namespace face
}; // namespace ndnc

#endif // NDNC_FACE_HPP
