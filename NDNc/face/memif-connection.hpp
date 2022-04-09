/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2022 California Institute of Technology
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

#ifndef NDNC_TRANSPORT_MEMIF_CONNECTION_HPP
#define NDNC_TRANSPORT_MEMIF_CONNECTION_HPP

extern "C" {
#include <libmemif.h>
}

namespace ndnc {
namespace face {
namespace transport {
typedef struct memif_connection {
    // memif connection handle
    memif_conn_handle_t conn_handle;
    uint8_t is_connected;
    // transmit queue id
    uint16_t tx_qid;
    // tx buffers
    memif_buffer_t *tx_bufs;
    // allocated tx buffers counter
    // number of tx buffers pointing to shared memory
    uint16_t tx_buf_num;
    // rx buffers
    memif_buffer_t *rx_bufs;
    // allocated rx buffers counter
    // number of rx buffers pointing to shared memory
    uint16_t rx_buf_num;
    // ndnc::face::transport
    void *transport;
} memif_connection_t;
}; // namespace transport
}; // namespace face
}; // namespace ndnc

#endif // NDNC_TRANSPORT_MEMIF_CONNECTION_HPP
