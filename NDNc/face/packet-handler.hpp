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

#ifndef NDNC_FACE_PACKET_HANDLER_HPP
#define NDNC_FACE_PACKET_HANDLER_HPP

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/lp/nack.hpp>
#include <ndn-cxx/lp/pit-token.hpp>

namespace ndnc {
class Face;
}
#include "face.hpp"

namespace ndnc {
class PacketHandler {
  public:
    explicit PacketHandler(Face &face);

  protected:
    virtual ~PacketHandler();

  public:
    /**
     * @brief
     * Consumer flow
     *
     * @param interest
     * @param rxQueue
     * @return true
     * @return false
     */
    virtual bool
    enqueueInterestPacket(std::shared_ptr<ndn::Interest> &&interest,
                          void *rxQueue);

    virtual bool
    enqueueInterests(std::vector<std::shared_ptr<ndn::Interest>> &&interests,
                     size_t n, void *rxQueue);

    /**
     * @brief
     * Consumer flow
     *
     * @param data
     * @param pitToken
     */
    virtual void dequeueDataPacket(std::shared_ptr<ndn::Data> &&data,
                                   ndn::lp::PitToken &&pitToken);

    /**
     * @brief
     * Consumer flow
     *
     * @param nack
     */
    virtual void dequeueNackPacket(std::shared_ptr<ndn::lp::Nack> &&nack,
                                   ndn::lp::PitToken &&pitToken);

    /**
     * @brief
     * Producer flow
     *
     * @param data
     * @param pitToken
     * @return true
     * @return false
     */
    virtual bool enqueueDataPacket(ndn::Data &&data,
                                   ndn::lp::PitToken &&pitToken);

    /**
     * @brief
     * Producer flow
     *
     * @param interest
     * @param pitToken
     */
    virtual void
    dequeueInterestPacket(std::shared_ptr<ndn::Interest> &&interest,
                          ndn::lp::PitToken &&pitToken);

  protected:
    Face *m_face;
    friend Face;
};
}; // namespace ndnc

#endif // NDNC_FACE_PACKET_HANDLER_HPP
