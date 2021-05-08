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

  private:
    /**
     * @brief Override to be invoked periodically
     *
     */
    virtual void loop();

  public:
    /**
     * @brief Override to send Interest packets
     *
     * @param interest the Interest packet to send
     * @return true packet has been sent
     * @return false packet could not be sent
     */
    virtual bool expressInterest(std::shared_ptr<const ndn::Interest> interest,
                                 ndn::lp::PitToken pitToken);

    /**
     * @brief Override to send Data packets
     *
     * @param data the Data packet to send
     * @param pitToken the PIT token of the Interest that this Data packet
     * satisfies
     * @return true
     * @return false
     */
    virtual bool putData(std::shared_ptr<ndn::Data> &data,
                         ndn::lp::PitToken pitToken);

    /**
     * @brief Override to receive Interest packets
     *
     * @param interest the Interest packet
     * @param pitToken the PIT token of this Interest
     */
    virtual void processInterest(std::shared_ptr<ndn::Interest> &interest,
                                 ndn::lp::PitToken pitToken);

    /**
     * @brief Override to receive Data packets
     *
     * @param data the Data packet
     */
    virtual void processData(std::shared_ptr<ndn::Data> &data,
                             ndn::lp::PitToken pitToken);

    /**
     * @brief Override to receive NACK packets
     *
     * @param nack the NACK packet
     */
    virtual void processNack(std::shared_ptr<ndn::lp::Nack> &nack);

  private:
    Face *m_face;
    friend Face;
};
}; // namespace ndnc

#endif // NDNC_FACE_PACKET_HANDLER_HPP
