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

#ifndef NDNC_FACE_MEMIF_HPP
#define NDNC_FACE_MEMIF_HPP

extern "C" {
#include <libmemif.h>
}
#include "logger/logger.hpp"
#include "transport.hpp"

namespace ndnc {
#ifndef MAX_MEMIF_TX_BUFS
/** @brief send burst size. */
#define MAX_MEMIF_TX_BUFS 256
#endif

#ifndef MAX_MEMIF_RX_BUFS
/** @brief receive burst size. */
#define MAX_MEMIF_RX_BUFS 1024
#endif

class Memif : public virtual Transport {
  public:
    using DefaultDataroom = std::integral_constant<uint16_t, 2048>;

  public:
    bool init(const char *socket_path, uint32_t id, const char *app_name = "",
              uint16_t max_pkt_len = DefaultDataroom::value) {
        deinit();
        this->m_max_pkt_len = max_pkt_len;

        if (!this->init_memif_socket(socket_path, app_name) ||
            this->m_socket == nullptr) {
            return false;
        }

        if (!this->init_memif(id) || this->m_conn == nullptr) {
            return false;
        }

        m_tx_bufs = (memif_buffer_t *)malloc(sizeof(memif_buffer_t) *
                                             MAX_MEMIF_TX_BUFS);
        m_rx_bufs = (memif_buffer_t *)malloc(sizeof(memif_buffer_t) *
                                             MAX_MEMIF_RX_BUFS);

        return true;
    }

    bool init_memif_socket(const char *socket_path, const char *app_name) {
        memif_socket_args_t *args =
            (memif_socket_args_t *)malloc(sizeof(memif_socket_args_t));

        args->alloc = NULL;
        args->free = NULL;
        args->on_control_fd_update = NULL;
        args->realloc = NULL;

#pragma GCC diagnostic ignored "-Wstringop-truncation"
        strncpy((char *)args->path, socket_path, 108);
        strncpy((char *)args->app_name, app_name, 32);
#pragma GCC diagnostic pop

        int err = memif_create_socket(&m_socket, args, this);
        free(args);

        if (err != MEMIF_ERR_SUCCESS) {
            LOG_ERROR("memif_create_socket err=%s", memif_strerror(err));
            deinit();
            return false;
        }

        return true;
    }

    bool init_memif(uint32_t id) {
        memif_conn_args_t args = {};

        args.socket = this->m_socket;
        args.interface_id = id;
        args.buffer_size = size_to_pow2(m_max_pkt_len);
        args.log2_ring_size = 12;

        int err = memif_create(&m_conn, &args, Memif::on_connect,
                               Memif::on_disconnect, Memif::on_interrupt, this);

        if (err != MEMIF_ERR_SUCCESS) {
            LOG_ERROR("memif_create err=%s", memif_strerror(err));
            deinit();
            return false;
        }

        return true;
    }

    void deinit() {
        if (m_conn != nullptr) {
            int err = memif_delete(&m_conn);
            if (err != MEMIF_ERR_SUCCESS) {
                LOG_WARN("memif_delete err=%s", memif_strerror(err));
            }
        }

        if (m_socket != nullptr)
            memif_delete_socket(&m_socket);

        if (m_tx_bufs != nullptr)
            free(m_tx_bufs);
        m_tx_bufs = NULL;

        if (m_rx_bufs != nullptr)
            free(m_rx_bufs);
        m_rx_bufs = NULL;
    }

    void loop() final {
        int err = memif_poll_event(m_socket, 0);
        if (err != MEMIF_ERR_SUCCESS) {
            LOG_WARN("memif_poll_event err=%s", memif_strerror(err));
        }
    }

    bool isUp() final {
        return m_up;
    }

    bool send(ndn::Block pkt) final {
        if (!isUp()) {
            LOG_ERROR("memif send drop=transport-disconnected");
            return false;
        }

        if (pkt.size() > m_max_pkt_len) {
            LOG_ERROR("memif send drop=pkt-too-long len=%li", pkt.size());
            return false;
        }

        memset(m_tx_bufs, 0, sizeof(memif_buffer_t));
        uint16_t nTx = 0;

        int err = memif_buffer_alloc(m_conn, 0, m_tx_bufs, 1, &nTx,
                                     size_to_pow2(pkt.size()));

        if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF_RING) {
            LOG_ERROR("memif_buffer_alloc allocated: %d/1 bufs. err=%s", nTx,
                      memif_strerror(err));
            return false;
        }

        std::copy_n(pkt.wire(), pkt.size(),
                    static_cast<uint8_t *>(m_tx_bufs[0].data));

        err = memif_tx_burst(m_conn, 0, m_tx_bufs, 1, &nTx);
        if (err != MEMIF_ERR_SUCCESS) {
            LOG_ERROR("memif_tx_burst transmitted: %d/1 pkts. err=%s", nTx,
                      memif_strerror(err));
            return false;
        }

        return true;
    }

    bool send(std::vector<ndn::Block> &&pkts, uint16_t n, uint16_t *nTx) final {
        *nTx = 0;

        if (!isUp()) {
            LOG_ERROR("memif send drop=transport-disconnected");
            return false;
        }

        if (n > MAX_MEMIF_TX_BUFS) {
            LOG_ERROR("memif send drop=max-burst-size-breach");
            return false;
        }

        size_t bsize = pkts.front().size();
        for (auto it = pkts.begin(); it != pkts.end(); ++it) {
            if (it->size() > bsize)
                bsize = it->size();
        }

        if (bsize > m_max_pkt_len) {
            LOG_ERROR("memif send drop=pkt-too-long len=%li", bsize);
            return false;
        }

        int err = memif_buffer_alloc(m_conn, 0, m_tx_bufs, n, nTx,
                                     size_to_pow2(bsize));

        if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF_RING) {
            LOG_ERROR("memif_buffer_alloc allocated: %d/%d bufs. err=%s", *nTx,
                      n - *nTx, memif_strerror(err));
            return false;
        }

        for (size_t i = 0; i < *nTx; ++i) {
            std::copy_n(pkts[i].wire(), pkts[i].size(),
                        static_cast<uint8_t *>(m_tx_bufs[i].data));
        }

        err = memif_tx_burst(m_conn, 0, m_tx_bufs, *nTx, nTx);
        if (err != MEMIF_ERR_SUCCESS) {
            LOG_ERROR("memif_tx_burst transmitted: %d/%d pkts. err=%s", *nTx,
                      n - *nTx, memif_strerror(err));
            return false;
        }

        return true;
    }

  private:
    void print_memif_details() {
        memif_details_t md;
        memset(&md, 0, sizeof(md));

        ssize_t buflen = 2048;
        char *buf = (char *)malloc(buflen);
        memset(buf, 0, buflen);

        int err = memif_get_details(this->m_conn, &md, buf, buflen);
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

    inline uint16_t size_to_pow2(size_t len) const {
        // libmemif internally assumes buffer_size to be power of two
        // https://github.com/FDio/vpp/blob/v21.10.1/extras/libmemif/src/main.c#L2406
        uint16_t size = 128;
        while (size < len)
            size <<= 1;
        return size;
    }

    static int on_connect(memif_conn_handle_t conn, void *self0) {
        auto self = reinterpret_cast<Memif *>(self0);
        self->m_up = true;

        LOG_DEBUG("memif connected");
        self->print_memif_details();

        memif_refill_queue(conn, 0, -1, 0);
        return 0;
    }

    static int on_disconnect(memif_conn_handle_t conn, void *self0) {
        auto self = reinterpret_cast<Memif *>(self0);

        if (self->m_conn != conn) {
            LOG_DEBUG("invalid connection");
        }

        self->m_up = false;

        LOG_DEBUG("memif disconnected");
        self->invokeDisconnectCallback();

        return 0;
    }

    static int on_interrupt(memif_conn_handle_t conn, void *self0,
                            uint16_t qid) {
        auto self = reinterpret_cast<Memif *>(self0);

        uint16_t nRx = 0;
        int err =
            memif_rx_burst(conn, qid, self->m_rx_bufs, MAX_MEMIF_RX_BUFS, &nRx);

        if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF) {
            LOG_ERROR("memif_rx_burst err=%s", memif_strerror(err));
            return 0;
        }

        for (uint16_t i = 0; i < nRx; ++i) {
            auto b = self->m_rx_bufs[i];
            self->invokeRxCallback(static_cast<const uint8_t *>(b.data), b.len);
        }

        memif_refill_queue(conn, qid, nRx, 0);
        return 0;
    }

  private:
    memif_socket_handle_t m_socket = nullptr;
    memif_conn_handle_t m_conn = nullptr;
    memif_buffer_t *m_tx_bufs;
    memif_buffer_t *m_rx_bufs;

    uint16_t m_max_pkt_len;

    bool m_up = false;
};
}; // namespace ndnc

#endif // NDNC_FACE_MEMIF_HPP
