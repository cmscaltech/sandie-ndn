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
#include <vector>

#include <ndn-cxx/encoding/block.hpp>

namespace ndnc {
namespace face {
namespace transport {
class Transport {
  public:
    using OnDisconnectCallback = void (*)(void *ctx);
    using OnReceiveCallback = void (*)(void *ctx, const uint8_t *pkt,
                                       size_t len);

  public:
    virtual bool connect() noexcept = 0;
    virtual bool isConnected() noexcept = 0;
    virtual bool loop() noexcept = 0;
    virtual int send(ndn::Block pkt) noexcept = 0;
    virtual int send(std::vector<ndn::Block> &&pkt, uint16_t n) noexcept = 0;

    void setOnDisconnectCallback(OnDisconnectCallback cb, void *ctx) noexcept {
        this->onDisconnectCtx = ctx;
        this->onDisconnect = cb;
    }

    void setOnReceiveCallback(OnReceiveCallback cb, void *ctx) noexcept {
        this->onReceiveCtx = ctx; // face object
        this->onReceive = cb;
    }

  protected:
    void disconnect() noexcept {
        if (onDisconnect != nullptr && onDisconnectCtx != nullptr) {
            onDisconnect(onDisconnectCtx);
        }
    }

    void receive(const uint8_t *pkt, size_t len) noexcept {
        if (onReceive != nullptr && onReceiveCtx != nullptr) {
            onReceive(onReceiveCtx, pkt, len);
        }
    }

  private:
    void *onDisconnectCtx = nullptr;
    void *onReceiveCtx = nullptr;

    OnDisconnectCallback onDisconnect = nullptr;
    OnReceiveCallback onReceive = nullptr;
};
}; // namespace transport
}; // namespace face
}; // namespace ndnc

#endif // NDNC_FACE_TRANSPORT_HPP
