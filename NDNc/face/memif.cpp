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
#include <thread>

#include "memif.hpp"

namespace ndnc {
namespace face {
namespace transport {
Memif::Memif(uint16_t dataroom, const char *socketPath, const char *appName)
    : m_dataroom{dataroom}, m_socket{nullptr}, m_conn{nullptr} {

    if (!this->createSocket(socketPath, appName)) {
        throw std::runtime_error("unable to create memif socket");
    }
}

Memif::~Memif() {
    disconnect();

    if (m_socket != nullptr) {
        memif_delete_socket(&m_socket);
    }
}

bool Memif::createSocket(const char *socketPath, const char *appName) {
    memif_socket_args_t memif_socket_args = {};
    memif_socket_args.alloc = nullptr;
    memif_socket_args.realloc = nullptr;
    memif_socket_args.free = nullptr;
    memif_socket_args.on_control_fd_update = nullptr;

#pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncpy((char *)(memif_socket_args.path), socketPath, 108);
    strncpy((char *)memif_socket_args.app_name, appName, 32);
#pragma GCC diagnostic pop

    auto err = memif_create_socket(&m_socket, &memif_socket_args, nullptr);

    if (err != MEMIF_ERR_SUCCESS) {
        LOG_FATAL("memif_create_socket err=%s", memif_strerror(err));
        return false;
    }

    return m_socket != nullptr;
}

memif_connection_t *Memif::createInterface(int id) {
    memif_conn_args_t memif_conn_args = {};
    memif_conn_args.socket = m_socket;
    memif_conn_args.interface_id = id; // local face id
    memif_conn_args.is_master = 0;
    memif_conn_args.buffer_size = Memif::pow2(m_dataroom);
    memif_conn_args.log2_ring_size = 12;
    memif_conn_args.mode = MEMIF_INTERFACE_MODE_ETHERNET;

    memif_connection_t *conn =
        (memif_connection_t *)malloc(sizeof(memif_connection_t));

    conn->conn_handle = nullptr;
    conn->transport = nullptr;

    auto err =
        memif_create(&conn->conn_handle, &memif_conn_args,
                     ndnc::face::transport::Memif::handleConnect,
                     ndnc::face::transport::Memif::handleDisconnect,
                     ndnc::face::transport::Memif::handleInterrupt, conn);

    if (err != MEMIF_ERR_SUCCESS) {
        LOG_ERROR("memif_create err=%s", memif_strerror(err));
        free(conn);
        return nullptr;
    }

    conn->tx_bufs =
        (memif_buffer_t *)malloc(sizeof(memif_buffer_t) * MAX_MEMIF_TX_BUFS);
    conn->tx_buf_num = 0;
    conn->tx_qid = 0;
    conn->rx_bufs =
        (memif_buffer_t *)malloc(sizeof(memif_buffer_t) * MAX_MEMIF_RX_BUFS);
    conn->rx_buf_num = 0;
    conn->transport = this;

    return conn;
}

bool Memif::connect() noexcept {
    m_conn = createInterface(0);

    if (m_conn == nullptr) {
        return false;
    }

    for (auto i = 0; !isConnected() && i < 1e4; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        loop();
    }

    if (!isConnected()) {
        LOG_FATAL("unable to connect to memif");
        return false;
    }

    return true;
}

bool Memif::isConnected() noexcept {
    return m_conn->is_connected;
}

void Memif::disconnect() noexcept {
    if (m_conn == nullptr) {
        return;
    }

    m_conn->is_connected = 0;

    if (m_conn->tx_bufs != nullptr) {
        free(m_conn->tx_bufs);
    }
    m_conn->tx_bufs = nullptr;
    m_conn->tx_buf_num = 0;

    if (m_conn->rx_bufs != nullptr) {
        free(m_conn->rx_bufs);
    }
    m_conn->rx_bufs = nullptr;
    m_conn->rx_buf_num = 0;

    m_conn->transport = nullptr;

    if (m_conn->conn_handle != nullptr) {
        memif_delete(&m_conn->conn_handle);
    }

    free(m_conn);
}

bool Memif::loop() noexcept {
    auto err = memif_poll_event(m_socket, 0);

    if (err != MEMIF_ERR_SUCCESS) {
        LOG_WARN("memif_poll_event err=%s", memif_strerror(err));
        return false;
    }

    return true;
}

int Memif::send(ndn::Block pkt) noexcept {
    if (!isConnected()) {
        LOG_ERROR("memif send drop=transport-disconnected");
        return -1;
    }

    if (m_conn->tx_buf_num >= MAX_MEMIF_TX_BUFS) {
        LOG_ERROR("memif send drop=max-memif-tx-bufs-exceeded num=%d",
                  m_conn->tx_buf_num);
        return -1;
    }

    if (pkt.size() > m_dataroom) {
        LOG_ERROR("memif send drop=pkt-too-long len=%li", pkt.size());
        return -1;
    }

    int err =
        memif_buffer_alloc(m_conn->conn_handle, m_conn->tx_qid, m_conn->tx_bufs,
                           1, &m_conn->tx_buf_num, pow2(pkt.size()));

    if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF_RING) {
        LOG_ERROR("memif_buffer_alloc allocated: %d/1 bufs. err=%s",
                  m_conn->tx_buf_num, memif_strerror(err));
        return -1;
    }

    std::copy_n(pkt.wire(), pkt.size(),
                static_cast<uint8_t *>(m_conn->tx_bufs[0].data));

    uint16_t tx = 0;
    err = memif_tx_burst(m_conn->conn_handle, m_conn->tx_qid, m_conn->tx_bufs,
                         m_conn->tx_buf_num, &tx);

    if (err != MEMIF_ERR_SUCCESS) {
        LOG_ERROR("memif_tx_burst transmitted: %d/1 pkts. err=%s", tx,
                  memif_strerror(err));
        return -1;
    }

    m_conn->tx_buf_num -= tx;

    if (m_conn->tx_buf_num > 0) {
        LOG_FATAL("memif_tx_burst err=failed-to-send-allocated-packets");
        return -1;
    }

    return tx;
}

int Memif::send(std::vector<ndn::Block> &&pkts, uint16_t n) noexcept {
    if (!isConnected()) {
        LOG_ERROR("memif send drop=transport-disconnected");
        return -1;
    }

    if (m_conn->tx_buf_num >= MAX_MEMIF_TX_BUFS) {
        LOG_ERROR("memif send drop=max-memif-tx-bufs-exceeded num=%d",
                  m_conn->tx_buf_num);
        return -1;
    }

    if (n > MAX_MEMIF_TX_BUFS) {
        LOG_ERROR("memif send drop=max-burst-size-breach");
        return -1;
    }

    size_t bsize = pkts.front().size();
    for (auto it = pkts.begin(); it != pkts.end(); ++it) {
        if (it->size() > bsize)
            bsize = it->size();
    }

    if (bsize > m_dataroom) {
        LOG_ERROR("memif send drop=pkt-too-long len=%li", bsize);
        return -1;
    }

    int err =
        memif_buffer_alloc(m_conn->conn_handle, m_conn->tx_qid, m_conn->tx_bufs,
                           n, &m_conn->tx_buf_num, pow2(bsize));

    if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF_RING) {
        LOG_ERROR("memif_buffer_alloc allocated: %d/%d bufs. err=%s",
                  m_conn->tx_buf_num, n - m_conn->tx_buf_num,
                  memif_strerror(err));
        return -1;
    }

    for (size_t i = 0; i < m_conn->tx_buf_num; ++i) {
        std::copy_n(pkts[i].wire(), pkts[i].size(),
                    static_cast<uint8_t *>(m_conn->tx_bufs[i].data));
    }

    uint16_t tx = 0;
    err = memif_tx_burst(m_conn->conn_handle, m_conn->tx_qid, m_conn->tx_bufs,
                         m_conn->tx_buf_num, &tx);
    if (err != MEMIF_ERR_SUCCESS) {
        LOG_ERROR("memif_tx_burst transmitted: %d/%d pkts. err=%s", tx, n - tx,
                  memif_strerror(err));
        return -1;
    }

    m_conn->tx_buf_num -= tx;

    if (m_conn->tx_buf_num > 0) {
        LOG_FATAL("memif_tx_burst err=failed-to-send-allocated-packets");
        return -1;
    }

    return tx;
}

int Memif::handleConnect(memif_conn_handle_t conn_handle, void *ctx) {
    auto conn = reinterpret_cast<memif_connection_t *>(ctx);

    if (conn->conn_handle != conn_handle) {
        LOG_WARN("memif_on_connect err=invalid-connection");
        return -1;
    }

    conn->is_connected = 1;

    LOG_DEBUG("memif connected");
    Memif::logDetails(conn);

    memif_refill_queue(conn_handle, 0, -1, 0);
    return 0;
}

int Memif::handleDisconnect(memif_conn_handle_t conn_handle, void *ctx) {
    auto conn = reinterpret_cast<memif_connection_t *>(ctx);

    if (conn->conn_handle != conn_handle) {
        LOG_WARN("memif_on_disconnect err=invalid-connection");
        return -1;
    }

    conn->is_connected = 0;

    LOG_DEBUG("memif disconnected");
    return 0;
}

int Memif::handleInterrupt(memif_conn_handle_t conn_handle, void *ctx,
                           uint16_t qid) {
    auto conn = reinterpret_cast<memif_connection_t *>(ctx);

    if (conn->conn_handle != conn_handle) {
        LOG_WARN("memif_on_interrupt err=invalid-connection");
        return -1;
    }

    int err = memif_rx_burst(conn_handle, qid, conn->rx_bufs, MAX_MEMIF_RX_BUFS,
                             &conn->rx_buf_num);

    if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF) {
        LOG_ERROR("memif_rx_burst err=%s", memif_strerror(err));
        return -1;
    }

    auto transport = reinterpret_cast<Memif *>(conn->transport);

    for (uint16_t i = 0; i < conn->rx_buf_num; ++i) {
        auto b = conn->rx_bufs[i];
        transport->receive(static_cast<const uint8_t *>(b.data), b.len);
    }

    memif_refill_queue(conn_handle, qid, conn->rx_buf_num, 0);
    return 0;
}

void Memif::logDetails(memif_connection_t *conn) {
    memif_details_t md;
    memset(&md, 0, sizeof(md));

    ssize_t buflen = 2048;
    char *buf = (char *)malloc(buflen);
    memset(buf, 0, buflen);

    int err = memif_get_details(conn->conn_handle, &md, buf, buflen);
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

inline uint16_t Memif::pow2(size_t len) {
    // libmemif internally assumes buffer_size to be power of two
    // https://github.com/FDio/vpp/blob/v21.10.1/extras/libmemif/src/main.c#L2406
    uint16_t size = 128;
    while (size < len)
        size <<= 1;
    return size;
}
}; // namespace transport
}; // namespace face
}; // namespace ndnc
