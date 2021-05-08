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

#ifndef NDNC_FACE_TRANSPORT_HPP
#define NDNC_FACE_TRANSPORT_HPP

#include <stddef.h>
#include <stdint.h>

namespace ndnc {
/**
 * @brief Base class of low-level transport.
 *
 */
class Transport {
    using RxCallback = void (*)(void *ctx, const uint8_t *pkt, size_t pktLen);
    using DisconnectCallback = void (*)(void *ctx);

  public:
    virtual ~Transport() = default;

    /**
     * @brief Determine whether transport is connected
     *
     * @return true
     * @return false
     */
    virtual bool isUp() const { return doIsUp(); }

    /**
     * @brief Process periodical events, such as receiving packets
     *
     */
    virtual void loop() { doLoop(); }

    /**
     * @brief Set the Rx Callback objectSet incoming packet callback
     *
     * @param cb
     * @param ctx
     */
    void setRxCallback(RxCallback cb, void *ctx) {
        m_rxCb = cb;
        m_rxCtx = ctx;
    }

    /**
     * @brief Synchronously transmit a packet
     *
     * @param pkt
     * @param pktLen
     * @return true
     * @return false
     */
    bool send(const uint8_t *pkt, size_t pktLen) { return doSend(pkt, pktLen); }

    void setDisconnectCallback(DisconnectCallback cb, void *ctx) {
        m_disconnectCb = cb;
        m_disconnectCtx = ctx;
    }

  protected:
    /**
     * @brief Invoke incoming packet callback for a received packet
     *
     * @param pkt
     * @param pktLen
     */
    void invokeRxCallback(const uint8_t *pkt, size_t pktLen) {
        m_rxCb(m_rxCtx, pkt, pktLen);
    }

    void invokeDisconnectCallback() { m_disconnectCb(m_disconnectCtx); }

  private:
    virtual bool doIsUp() const = 0;

    virtual void doLoop() = 0;

    virtual bool doSend(const uint8_t *pkt, size_t pktLen) = 0;

  private:
    RxCallback m_rxCb = nullptr;
    DisconnectCallback m_disconnectCb = nullptr;

    void *m_rxCtx = nullptr;
    void *m_disconnectCtx = nullptr;
};
}; // namespace ndnc

#endif // NDNC_FACE_TRANSPORT_HPP
