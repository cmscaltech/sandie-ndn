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

#include <array>
#include <iostream>

#include "transport.hpp"

extern "C" {
#include <libmemif.h>
}

#ifndef NDNC_MEMIF_RXBURST
/** @brief Receive burst size. */
#define NDNC_MEMIF_RXBURST 256
#endif

namespace ndnc {
/**
 * @brief A transport that communicates via libmemif.
 *
 */
class Memif : public virtual Transport {
  public:
    explicit Memif(uint16_t maxPktLen = 8800) : m_maxPktLen(maxPktLen) {}

    bool begin(const char *socketName, uint32_t id) {
        end();

        int err = memif_init(nullptr, const_cast<char *>("NDNc"), nullptr,
                             nullptr, nullptr);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_init: " << memif_strerror(err) << "\n";
            return false;
        }
        m_init = true;

        err = memif_create_socket(&m_sock, socketName, nullptr);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_create_socket: " << memif_strerror(err)
                      << "\n";
            return false;
        }

        memif_conn_args_t args = {};
        args.socket = m_sock;
        args.interface_id = id;
        for (args.buffer_size = 64; args.buffer_size < m_maxPktLen;) {
            args.buffer_size <<= 1;
            // libmemif internally assumes buffer_size to be power of two
            // https://github.com/FDio/vpp/blob/v21.06/extras/libmemif/src/main.c#L2406
        }
        err =
            memif_create(&m_conn, &args, Memif::handleConnect,
                         Memif::handleDisconnect, Memif::handleInterrupt, this);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_create: " << memif_strerror(err) << "\n";
            return false;
        }

        return true;
    }

    bool end() {
        if (m_conn != nullptr) {
            int err = memif_delete(&m_conn);
            if (err != MEMIF_ERR_SUCCESS) {
                std::cout << "ERROR: memif_delete: " << memif_strerror(err)
                          << "\n";
                return false;
            }
        }

        if (m_sock != nullptr) {
            int err = memif_delete_socket(&m_sock);
            if (err != MEMIF_ERR_SUCCESS) {
                std::cout << "ERROR: memif_delete_socket: "
                          << memif_strerror(err) << "\n";
                return false;
            }
        }

        if (m_init) {
            int err = memif_cleanup();
            if (err != MEMIF_ERR_SUCCESS) {
                std::cout << "ERROR: memif_cleanup: " << memif_strerror(err)
                          << "\n";
                return false;
            }
            m_init = false;
        }
        return true;
    }

  private:
    bool doIsUp() const final { return m_isUp; }

    void doLoop() final {
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

    bool doSend(const uint8_t *pkt, size_t pktLen) final {
        if (!m_isUp) {
            std::cout << "ERROR: Memif send drop=transport-disconnected\n";
            return false;
        }

        if (pktLen > m_maxPktLen) {
            std::cout << "ERROR: Memif send drop=pkt-too-long len= " << pktLen
                      << "\n";
            return false;
        }

        memif_buffer_t b = {};
        uint16_t nAlloc = 0;
        int err = memif_buffer_alloc(m_conn, 0, &b, 1, &nAlloc, pktLen);
        if (err != MEMIF_ERR_SUCCESS || nAlloc != 1) {
            std::cout << "ERROR: memif_buffer_alloc: " << memif_strerror(err)
                      << "\n";
            return false;
        }

        std::copy_n(pkt, pktLen, static_cast<uint8_t *>(b.data));
        b.len = pktLen;

        uint16_t nTx = 0;
        err = memif_tx_burst(m_conn, 0, &b, 1, &nTx);
        if (err != MEMIF_ERR_SUCCESS || nTx != 1) {
            std::cout << "ERROR: memif_tx_burst: " << memif_strerror(err)
                      << "\n";
            return false;
        }
        return true;
    }

    static int handleConnect(memif_conn_handle_t conn, void *self0) {
        Memif *self = reinterpret_cast<Memif *>(self0);
        assert(self->m_conn == conn);
        self->m_isUp = true;

        std::cout << "INFO: Memif connected\n";

        int err = memif_refill_queue(conn, 0, -1, 0);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_refill_queue: " << memif_strerror(err)
                      << "\n";
            return -1;
        }

        return 0;
    }

    static int handleDisconnect(memif_conn_handle_t conn, void *self0) {
        Memif *self = reinterpret_cast<Memif *>(self0);
        assert(self->m_conn == conn);
        self->m_isUp = false;

        std::cout << "INFO: Memif disconnected\n";
        self->invokeDisconnectCallback();
        return 0;
    }

    static int handleInterrupt(memif_conn_handle_t conn, void *self0,
                               uint16_t qid) {
        Memif *self = reinterpret_cast<Memif *>(self0);
        assert(self->m_conn == conn);

        std::array<memif_buffer_t, NDNC_MEMIF_RXBURST> burst{};
        uint16_t nRx = 0;
        int err = memif_rx_burst(conn, qid, burst.data(), burst.size(), &nRx);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_rx_burst: " << memif_strerror(err)
                      << "\n";
            return 0;
        }

        for (uint16_t i = 0; i < nRx; ++i) {
            const memif_buffer_t &b = burst[i];
            self->invokeRxCallback(static_cast<const uint8_t *>(b.data), b.len);
        }

        err = memif_refill_queue(conn, qid, nRx, 0);
        if (err != MEMIF_ERR_SUCCESS) {
            std::cout << "ERROR: memif_refill_queue: " << memif_strerror(err)
                      << "\n";
        }
        return 0;
    }

  public:
    memif_socket_handle_t m_sock = nullptr;
    memif_conn_handle_t m_conn = nullptr;
    uint16_t m_maxPktLen;
    bool m_init = false;
    bool m_isUp = false;
};
}; // namespace ndnc

#endif // NDNC_FACE_MEMIF_HPP
