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

#ifndef NDNC_PACKET_HANDLER_HH
#define NDNC_PACKET_HANDLER_HH

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>

namespace ndnc {
class Face;
}
#include "face.hpp"

namespace ndnc {
class PacketHandler : public std::enable_shared_from_this<PacketHandler> {
    public:
    explicit PacketHandler() = default;
    explicit PacketHandler(std::shared_ptr<Face> face);

    protected:
    virtual ~PacketHandler();

    private:
    /** @brief Override to be invoked periodically. */
    virtual void loop();

    public:
    /**
     * @brief Override to receive Interest packets.
     * @retval true packet has been accepted by this handler.
     * @retval false packet is not accepted, and should go to the next handler.
     */
    virtual bool processInterest(std::shared_ptr<ndn::Interest>& interest);

    /**
     * @brief Override to receive Data packets.
     * @retval true packet has been accepted by this handler.
     * @retval false packet is not accepted, and should go to the next handler.
     */
    virtual bool processData(std::shared_ptr<ndn::Data>& data);

    /**
     * @brief Override to receive Nack packets.
     * @retval true packet has been accepted by this handler.
     * @retval false packet is not accepted, and should go to the next handler.
     */
    // virtual bool processNack(Nack);

    private:
    std::shared_ptr<Face> m_face;
};
}; // namespace ndnc

#endif // NDNC_PACKET_HANDLER_HH