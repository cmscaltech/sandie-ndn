/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018 California Institute of Technology                        *
 *                                                                            *
 * Author: Catalin Iordache <catalin.iordache@cern.ch>                        *
 *                                                                            *
 * This program is free software: you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.     *
 *****************************************************************************/

#ifndef XRDNDN_DISPATCHER
#define XRDNDN_DISPATCHER

#include <math.h>
#include <ndn-cxx/face.hpp>

#include "xrdndn-common.hh"

using namespace ndn;

namespace xrdndn {
class Packager {
  public:
    Packager() {}
    ~Packager() {}

    // Prepare Data containing an non/negative integer
    std::shared_ptr<Data> getPackage(const Name &name, int value) {

        int type = value < 0 ? xrdndn::tlv::negativeInteger
                             : xrdndn::tlv::nonNegativeInteger;
        const Block content =
            makeNonNegativeIntegerBlock(ndn::tlv::Content, fabs(value));

        std::shared_ptr<Data> data = std::make_shared<Data>(name);
        data->setContent(content);
        data->setContentType(type);
        data->setFreshnessPeriod(DEFAULT_FRESHNESS_PERIOD);
        m_keyChain.sign(*data); // signWithDigestSha256
        return data;
    }

    // Prepare Data as bytes
    std::shared_ptr<Data> getPackage(const Name &name, const uint8_t *value,
                                     ssize_t size) {
        std::shared_ptr<Data> data = std::make_shared<Data>(name);
        data->setContent(value, size);
        data->setFreshnessPeriod(DEFAULT_FRESHNESS_PERIOD);
        m_keyChain.sign(*data); // signWithDigestSha256
        return data;
    }

  private:
    KeyChain m_keyChain;
};

class Dispatcher {
  public:
    Dispatcher(Face *face) {
        m_face = face;
        m_packager = std::make_shared<Packager>();
    }

    ~Dispatcher() {}

    // Send Data
    void send(std::shared_ptr<Data> data) { m_face->put(*data); }

    // Send integer as Data
    void sendInteger(const Name &name, int value) {
        // NDN_LOG_TRACE("Sending integer.");
        this->send(m_packager->getPackage(name, value));
    }

    // Send string as Data
    void sendString(const Name &name, std::string value, ssize_t size) {
        // NDN_LOG_TRACE("Sending string.");
        this->send(m_packager->getPackage(
            name, reinterpret_cast<const uint8_t *>(value.data()), size));
    }

  private:
    Face *m_face;
    std::shared_ptr<Packager> m_packager;
};
} // namespace xrdndn

#endif // XRDNDN_DISPATCHER