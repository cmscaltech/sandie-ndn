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

#include <iostream>

#include "transport.hpp"

extern "C" {
#include <libmemif.h>
}

#ifndef NDNC_MAX_MEMIF_BUFS
/** @brief send/receive burst size. */
#define NDNC_MAX_MEMIF_BUFS 256
#endif

namespace ndnc {
class Memif : public virtual Transport {
  public:
    using DefaultDataroom = std::integral_constant<uint16_t, 2048>;

  public:
    bool init(const char *socket_name, uint32_t id, const char *name = "",
              uint16_t max_pkt_len = DefaultDataroom::value) {
        deinit();
        m_max_pkt_len = max_pkt_len;

        int err = memif_init(nullptr, const_cast<char *>("NDNc"), nullptr,
                             nullptr, nullptr);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_init: " << memif_strerror(err) << "\n";
            return false;
        } else {
            m_init = true;
        }

        err = memif_create_socket(&m_socket, socket_name, nullptr);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_create_socket: " << memif_strerror(err)
                      << "\n";
            return false;
        }

        memif_conn_args_t args = {};
        args.socket = m_socket;
        args.interface_id = id;
        strncpy((char *)args.interface_name, name, strlen(name));
        args.buffer_size = buffer_size(m_max_pkt_len);

        err = memif_create(&m_conn, &args, Memif::on_connect,
                           Memif::on_disconnect, Memif::on_interrupt, this);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_create: " << memif_strerror(err) << "\n";
            deinit();
            return false;
        }

        m_tx_bufs = (memif_buffer_t *)malloc(sizeof(memif_buffer_t) *
                                             NDNC_MAX_MEMIF_BUFS);
        m_rx_bufs = (memif_buffer_t *)malloc(sizeof(memif_buffer_t) *
                                             NDNC_MAX_MEMIF_BUFS);
        return true;
    }

    void deinit() {
        if (m_conn != nullptr)
            memif_delete(&m_conn);

        if (m_socket != nullptr)
            memif_delete_socket(&m_socket);

        if (m_init) {
            memif_cleanup();
            m_init = false;
        }

        if (m_tx_bufs != nullptr)
            free(m_tx_bufs);
        m_tx_bufs = NULL;

        if (m_rx_bufs != nullptr)
            free(m_rx_bufs);
        m_rx_bufs = NULL;
    }

  public:
    void loop() final {
        if (!m_init) {
            std::cout << "ERROR: m_init is null\n";
            return;
        }

        int err = memif_poll_event(0);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_poll_event: " << memif_strerror(err)
                      << "\n";
        }
    }

    bool isUp() const final { return m_up; }

    // send one packet
    bool send(Request req) const final {
        if (!isUp()) {
            std::cout << "ERROR: Memif send drop=transport-disconnected\n";
            return false;
        }

        if (req.size() > m_max_pkt_len) {
            std::cout << "ERROR: Memif send drop=pkt-too-long len="
                      << req.size() << "\n";
            return false;
        }

        memset(m_tx_bufs, 0, sizeof(memif_buffer_t));
        uint16_t nTx = 0;

        int err = memif_buffer_alloc(m_conn, 0, m_tx_bufs, 1, &nTx,
                                     buffer_size(req.size()));
        if (err != MEMIF_ERR_SUCCESS || nTx != 1) {
            std::cout << "ERROR: memif_buffer_alloc: " << memif_strerror(err)
                      << "\n";
            return false;
        }

        std::copy_n(req.wire(), req.size(),
                    static_cast<uint8_t *>(m_tx_bufs[0].data));

        err = memif_tx_burst(m_conn, 0, m_tx_bufs, 1, &nTx);
        if (err != MEMIF_ERR_SUCCESS || nTx != 1) {
            std::cout << "ERROR: memif_tx_burst: " << memif_strerror(err)
                      << "\n";
            return false;
        }

        return true;
    }

    // send n packets
    bool send(std::vector<Request> reqs) const final {
        if (!isUp()) {
            std::cout << "ERROR: Memif send drop=transport-disconnected\n";
            return false;
        }

        if (reqs.size() > NDNC_MAX_MEMIF_BUFS) {
            std::cout << "ERROR: Maximum burts size breached\n";
            return false;
        }

        size_t bsize = reqs.front().size();
        for (auto it = reqs.begin(); it != reqs.end(); ++it) {
            if (it->size() > bsize)
                bsize = it->size();
        }

        if (bsize > m_max_pkt_len) {
            std::cout << "ERROR: Memif send drop=pkt-too-long len=" << bsize
                      << "\n";
            return false;
        }

        bsize = buffer_size(bsize);

        uint16_t nTx = 0;
        memset(m_tx_bufs, 0, sizeof(memif_buffer_t) * reqs.size());

        int err =
            memif_buffer_alloc(m_conn, 0, m_tx_bufs, reqs.size(), &nTx, bsize);
        if (err != MEMIF_ERR_SUCCESS || nTx != reqs.size()) {
            std::cout << "ERROR: memif_buffer_alloc: " << memif_strerror(err)
                      << "\n";
            return false;
        }

        for (size_t i = 0; i < reqs.size(); ++i) {
            std::copy_n(reqs[i].wire(), reqs[i].size(),
                        static_cast<uint8_t *>(m_tx_bufs[i].data));
        }

        err = memif_tx_burst(m_conn, 0, m_tx_bufs, reqs.size(), &nTx);
        if (err != MEMIF_ERR_SUCCESS || nTx != reqs.size()) {
            std::cout << "ERROR: memif_tx_burst: " << memif_strerror(err)
                      << "\n";
            return false;
        }

        return true;
    }

  private:
    inline uint16_t buffer_size(size_t len) const {
        uint16_t size = 128;
        // libmemif internally assumes buffer_size to be power of two
        // https://github.com/FDio/vpp/blob/v21.06/extras/libmemif/src/main.c#L2406
        while (size < len)
            size <<= 1;
        return size;
    }

    static int on_connect(memif_conn_handle_t conn, void *self0) {
        auto self = reinterpret_cast<Memif *>(self0);
        assert(self->m_conn == conn);
        self->m_up = true;

        std::cout << "TRACE: Memif connected\n";

        int err = memif_refill_queue(conn, 0, -1, 0);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_refill_queue: " << memif_strerror(err)
                      << "\n";
            return -1;
        }

        return 0;
    }

    static int on_disconnect(memif_conn_handle_t conn, void *self0) {
        auto self = reinterpret_cast<Memif *>(self0);
        assert(self->m_conn == conn);
        self->m_up = false;

        std::cout << "TRACE: Memif disconnected\n";
        self->invokeDisconnectCallback();
        return 0;
    }

    static int on_interrupt(memif_conn_handle_t conn, void *self0,
                            uint16_t qid) {
        auto self = reinterpret_cast<Memif *>(self0);
        assert(self->m_conn == conn);

        uint16_t nRx = 0;

        int err = memif_rx_burst(conn, qid, self->m_rx_bufs,
                                 NDNC_MAX_MEMIF_BUFS, &nRx);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_rx_burst: " << memif_strerror(err)
                      << "\n";
            return 0;
        }

        for (uint16_t i = 0; i < nRx; ++i) {
            auto b = self->m_rx_bufs[i];
            self->invokeRxCallback(static_cast<const uint8_t *>(b.data), b.len);
        }

        err = memif_refill_queue(conn, qid, nRx, 0);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_refill_queue: " << memif_strerror(err)
                      << "\n";
            return -1;
        }

        return 0;
    }

  private:
    memif_socket_handle_t m_socket = nullptr;
    memif_conn_handle_t m_conn = nullptr;
    uint16_t m_max_pkt_len;
    bool m_init = false;
    bool m_up = false;

    memif_buffer_t *m_tx_bufs;
    memif_buffer_t *m_rx_bufs;
};
}; // namespace ndnc

#endif // NDNC_FACE_MEMIF_HPP
