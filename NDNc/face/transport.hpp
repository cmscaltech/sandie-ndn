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
class Transport {
  public:
    using PrivateContext = void *;

    using OnDisconnectCallback = void (*)(PrivateContext ctx);
    using OnReceiveCallback = void (*)(PrivateContext ctx, const uint8_t *pkt,
                                       size_t pktLen);

  public:
    virtual bool isUp() = 0;
    virtual void loop() = 0;

    virtual bool send(ndn::Block) = 0;
    virtual bool send(std::vector<ndn::Block> &&, uint16_t, uint16_t *) = 0;

    void setPrivateContext(PrivateContext ctx) {
        this->context = ctx;
    }

    void setOnDisconnectCallback(OnDisconnectCallback cb) {
        onDisconnectCallback = cb;
    }

    void setOnReceiveCallback(OnReceiveCallback cb) {
        onReceiveCallback = cb;
    }

    void onDisconnect() {
        if (onDisconnectCallback != nullptr) {
            onDisconnectCallback(context);
        }
    }

    void onReceive(const uint8_t *pkt, size_t pktLen) {
        onReceiveCallback(context, pkt, pktLen);
    }

  private:
    PrivateContext context = nullptr;

    OnDisconnectCallback onDisconnectCallback = nullptr;
    OnReceiveCallback onReceiveCallback = nullptr;
};
}; // namespace ndnc

#endif // NDNC_FACE_TRANSPORT_HPP
