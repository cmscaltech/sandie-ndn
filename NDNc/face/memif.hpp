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

#ifndef NDNC_TRANSPORT_MEMIF_HPP
#define NDNC_TRANSPORT_MEMIF_HPP

#include "logger/logger.hpp"
#include "memif-connection.hpp"
#include "transport.hpp"

namespace ndnc {
namespace face {
namespace transport {
#define MAX_MEMIF_TX_BUFS 256  // send burst size
#define MAX_MEMIF_RX_BUFS 1024 // receive burst size

class Memif : public Transport {
  public:
    Memif(uint16_t dataroom = 2048, const char *socketPath = "",
          const char *appName = "");
    ~Memif();

    void *getInterface(uint32_t id) final;

    bool loop() final;

    bool isConnected(void *ctx) final;

    int send(ndn::Block pkt, void *ctx) final;

    int send(std::vector<ndn::Block> &&pkts, uint16_t n, void *ctx) final;

  private:
    bool createSocket(const char *socketPath, const char *appName);

    memif_connection_t *createInterface(uint32_t id);

  private:
    static int on_connect(memif_conn_handle_t handle, void *ctx);

    static int on_disconnect(memif_conn_handle_t handle, void *ctx);

    static int on_interrupt(memif_conn_handle_t handle, void *ctx,
                            uint16_t qid);

    static uint16_t size_to_pow2(size_t len);

    static void print_memif_details(memif_connection_t *conn);

  private:
    uint16_t m_dataroom;
    uint8_t m_conn_count;
    memif_socket_handle_t m_socket;
    memif_connection_t *m_conn[32];
};
}; // namespace transport
}; // namespace face
}; // namespace ndnc

#endif // NDNC_TRANSPORT_MEMIF_CONNECTION_HPP
