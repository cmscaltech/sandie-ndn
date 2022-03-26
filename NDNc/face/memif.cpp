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

#include <stdexcept>

#include "memif.hpp"

namespace ndnc {
namespace face {
namespace transport {
Memif::Memif(uint16_t dataroom, const char *socketPath, const char *appName) {
    m_dataroom = dataroom;
    m_conn_count = 0;

    if (!this->createSocket(socketPath, appName)) {
        throw std::runtime_error("unable to create memif socket");
    }
}

Memif::~Memif() {
    for (auto i = 0; i < 32; ++i) {
        if (m_conn[i]->tx_bufs != nullptr) {
            free(m_conn[i]->tx_bufs);
        }

        m_conn[i]->tx_bufs = nullptr;
        m_conn[i]->tx_buf_num = 0;

        if (m_conn[i]->rx_bufs != nullptr) {
            free(m_conn[i]->rx_bufs);
        }

        m_conn[i]->rx_bufs = nullptr;
        m_conn[i]->rx_buf_num = 0;

        m_conn[i]->is_connected = 0;
        m_conn[i]->transport = nullptr;

        if (this->m_conn[i] != nullptr) {
            memif_delete(&m_conn[i]->handle);
        }
    }

    if (m_socket != nullptr) {
        memif_delete_socket(&m_socket);
    }
}

bool Memif::createSocket(const char *socketPath, const char *appName) {
    memif_socket_args_t *memif_socket_args = {0};

    memif_socket_args->path[0] = '@';
    strncpy(memif_socket_args->path + 1, socketPath, strlen(socketPath));
    strncpy(memif_socket_args->app_name, appName, strlen(appName));

    auto err = memif_create_socket(&this->m_socket, memif_socket_args, NULL);
    if (err != MEMIF_ERR_SUCCESS) {
        LOG_FATAL("memif_create_socket err=%s", memif_strerror(err));
        return false;
    }

    return true;
}

memif_connection_t *Memif::createInterface(uint32_t id) {
    if (m_conn_count >= 32) {
        LOG_ERROR("memif_create err=number of maximum memif connections "
                  "(32) has been reached");
        return nullptr;
    }

    memif_connection_t *conn;
    memset(&conn, 0, sizeof(conn));

    memif_conn_args_t memif_conn_args = {0};
    memif_conn_args.socket = m_socket;
    memif_conn_args.interface_id = id;
    memif_conn_args.is_master = 0;
    memif_conn_args.buffer_size = Memif::size_to_pow2(m_dataroom);
    memif_conn_args.log2_ring_size = 12;
    conn->transport = this;

    auto err =
        memif_create(&conn->handle, &memif_conn_args, Memif::on_connect,
                     Memif::on_disconnect, Memif::on_interrupt, (void *)conn);

    if (err != MEMIF_ERR_SUCCESS) {
        LOG_ERROR("memif_create err=%s", memif_strerror(err));
        return nullptr;
    }

    conn->tx_bufs =
        (memif_buffer_t *)malloc(sizeof(memif_buffer_t) * MAX_MEMIF_TX_BUFS);
    conn->tx_buf_num = 0;
    conn->rx_bufs =
        (memif_buffer_t *)malloc(sizeof(memif_buffer_t) * MAX_MEMIF_RX_BUFS);
    conn->rx_buf_num = 0;

    conn->index = m_conn_count;

    m_conn[conn->index] = conn;
    m_conn_count += 1;

    return conn;
}

void *Memif::getInterface(uint32_t id) {
    return this->createInterface(id);
}

bool Memif::loop() {
    auto err = memif_poll_event(m_socket, 0);

    if (err != MEMIF_ERR_SUCCESS) {
        LOG_WARN("memif_poll_event err=%s", memif_strerror(err));
        return false;
    }

    return true;
}

bool Memif::isConnected(void *ctx) {
    auto conn = reinterpret_cast<memif_connection_t *>(ctx);
    return conn->is_connected;
}

int Memif::send(ndn::Block pkt, void *ctx) {
    if (!isConnected(ctx)) {
        LOG_ERROR("memif send drop=transport-disconnected");
        return 0;
    }

    if (pkt.size() > m_dataroom) {
        LOG_ERROR("memif send drop=pkt-too-long len=%li", pkt.size());
        return 0;
    }

    auto conn = reinterpret_cast<memif_connection_t *>(ctx);

    int err = memif_buffer_alloc(conn->handle, 0, conn->tx_bufs, 1,
                                 &conn->tx_buf_num, size_to_pow2(pkt.size()));

    if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF_RING) {
        LOG_ERROR("memif_buffer_alloc allocated: %d/1 bufs. err=%s",
                  conn->tx_buf_num, memif_strerror(err));
        return 0;
    }

    std::copy_n(pkt.wire(), pkt.size(),
                static_cast<uint8_t *>(conn->tx_bufs[0].data));

    uint16_t tx = 0;
    err = memif_tx_burst(m_conn, 0, conn->tx_bufs, conn->tx_buf_num, &tx);

    if (err != MEMIF_ERR_SUCCESS) {
        LOG_ERROR("memif_tx_burst transmitted: %d/1 pkts. err=%s", tx,
                  memif_strerror(err));
        return 0;
    }

    conn->tx_buf_num -= tx;

    if (conn->tx_buf_num > 0) {
        LOG_FATAL("memif failed to send allocated packets");
        return 0;
    }

    return tx;
}

int Memif::send(std::vector<ndn::Block> &&pkts, uint16_t n, void *ctx) {
    if (!isConnected(ctx)) {
        LOG_ERROR("memif send drop=transport-disconnected");
        return 0;
    }

    if (n > MAX_MEMIF_TX_BUFS) {
        LOG_ERROR("memif send drop=max-burst-size-breach");
        return 0;
    }

    size_t bsize = pkts.front().size();
    for (auto it = pkts.begin(); it != pkts.end(); ++it) {
        if (it->size() > bsize)
            bsize = it->size();
    }

    if (bsize > m_dataroom) {
        LOG_ERROR("memif send drop=pkt-too-long len=%li", bsize);
        return 0;
    }

    auto conn = reinterpret_cast<memif_connection_t *>(ctx);

    int err = memif_buffer_alloc(conn, 0, conn->tx_bufs, n, &conn->tx_buf_num,
                                 size_to_pow2(bsize));

    if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF_RING) {
        LOG_ERROR("memif_buffer_alloc allocated: %d/%d bufs. err=%s",
                  conn->tx_buf_num, n - conn->tx_buf_num, memif_strerror(err));
        return 0;
    }

    for (size_t i = 0; i < conn->tx_buf_num; ++i) {
        std::copy_n(pkts[i].wire(), pkts[i].size(),
                    static_cast<uint8_t *>(conn->tx_bufs[i].data));
    }

    uint16_t tx = 0;
    err = memif_tx_burst(conn, 0, conn->tx_bufs, conn->tx_buf_num, &tx);
    if (err != MEMIF_ERR_SUCCESS) {
        LOG_ERROR("memif_tx_burst transmitted: %d/%d pkts. err=%s", tx, n - tx,
                  memif_strerror(err));
        return 0;
    }

    conn->tx_buf_num -= tx;

    if (conn->tx_buf_num > 0) {
        LOG_FATAL("memif failed to send allocated packets");
        return 0;
    }

    return tx;
}

int Memif::on_connect(memif_conn_handle_t handle, void *ctx) {
    auto conn = reinterpret_cast<memif_connection_t *>(ctx);
    conn->is_connected = 1;

    LOG_DEBUG("memif connected");
    Memif::print_memif_details(conn);

    memif_refill_queue(handle, 0, -1, 0);
    return 0;
}

int Memif::on_disconnect(memif_conn_handle_t handle, void *ctx) {
    auto conn = reinterpret_cast<memif_connection_t *>(ctx);

    if (conn->handle != handle) {
        LOG_DEBUG("invalid connection");
    }

    conn->is_connected = 0;
    LOG_DEBUG("memif disconnected");
    return 0;
}

int Memif::on_interrupt(memif_conn_handle_t handle, void *ctx, uint16_t qid) {
    auto conn = reinterpret_cast<memif_connection_t *>(ctx);

    int err = memif_rx_burst(handle, qid, conn->rx_bufs, MAX_MEMIF_RX_BUFS,
                             &conn->rx_buf_num);

    if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF) {
        LOG_ERROR("memif_rx_burst err=%s", memif_strerror(err));
        return 0;
    }

    auto transport = reinterpret_cast<Memif *>(conn->transport);
    for (uint16_t i = 0; i < conn->rx_buf_num; ++i) {
        auto b = conn->rx_bufs[i];
        transport->onReceive(static_cast<const uint8_t *>(b.data), b.len);
    }

    memif_refill_queue(handle, qid, conn->rx_buf_num, 0);
    return 0;
}

inline uint16_t Memif::size_to_pow2(size_t len) {
    // libmemif internally assumes buffer_size to be power of two
    // https://github.com/FDio/vpp/blob/v21.10.1/extras/libmemif/src/main.c#L2406
    uint16_t size = 128;
    while (size < len)
        size <<= 1;
    return size;
}

void Memif::print_memif_details(memif_connection_t *conn) {
    memif_details_t md;
    memset(&md, 0, sizeof(md));

    ssize_t buflen = 2048;
    char *buf = (char *)malloc(buflen);
    memset(buf, 0, buflen);

    int err = memif_get_details(conn->handle, &md, buf, buflen);
    if (err != MEMIF_ERR_SUCCESS) {
        LOG_WARN("memif_get_details err=%s", memif_strerror(err));
        if (err == MEMIF_ERR_NOCONN) {
            free(buf);
            return;
        }
    }

    LOG_INFO("memif details. app name: %s", (char *)md.inst_name);
    LOG_INFO("memif details. interface name: %s", (char *)md.if_name);
    LOG_INFO("memif details. id: %u", md.id);
    LOG_INFO("memif details. secret: %s", (char *)md.secret);
    LOG_INFO("memif details. role: %s", md.role ? "slave" : "master");
    LOG_INFO("memif details. mode: %s", md.mode == 0   ? "ethernet"
                                        : md.mode == 1 ? "ip"
                                        : md.mode == 2 ? "punt/inject"
                                                       : "unknown");
    LOG_INFO("memif details. socket path: %s", (char *)md.socket_path);

    for (auto i = 0; i < md.rx_queues_num; ++i) {
        LOG_INFO("memif details. rx_queue(%d) queue id: %u", i,
                 md.rx_queues[i].qid);
        LOG_INFO("memif details. rx_queue(%d) ring size: %u", i,
                 md.rx_queues[i].ring_size);
        LOG_INFO("memif details. rx_queue(%d) buffer size: %u", i,
                 md.rx_queues[i].buffer_size);
    }

    for (auto i = 0; i < md.tx_queues_num; ++i) {
        LOG_INFO("memif details. tx_queue(%d) queue id: %u", i,
                 md.tx_queues[i].qid);
        LOG_INFO("memif details. tx_queue(%d) ring size: %u", i,
                 md.tx_queues[i].ring_size);
        LOG_INFO("memif details. tx_queue(%d) buffer size: %u", i,
                 md.tx_queues[i].buffer_size);
    }

    LOG_INFO("memif details. link: %s", md.link_up_down ? "up" : "down");
    free(buf);
}
}; // namespace transport
}; // namespace face
}; // namespace ndnc
