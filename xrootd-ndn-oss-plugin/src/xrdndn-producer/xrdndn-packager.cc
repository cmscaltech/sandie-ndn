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

#include <math.h>

#include "../common/xrdndn-common.hh"
#include "../common/xrdndn-logger.hh"
#include "xrdndn-packager.hh"

using namespace ndn;

namespace xrdndnproducer {
Packager::Packager() {}
Packager::~Packager() {}

void Packager::digest(std::shared_ptr<ndn::Data> &data) {
    data->setFreshnessPeriod(DEFAULT_FRESHNESS_PERIOD);
    m_keyChain.sign(*data); // signWithDigestSha256
}

// Prepare Data containing an non/negative integer
std::shared_ptr<Data> Packager::getPackage(const ndn::Name &name, int value) {
    int type = value < 0 ? xrdndn::tlv::negativeInteger
                         : xrdndn::tlv::nonNegativeInteger;
    const Block content =
        makeNonNegativeIntegerBlock(ndn::tlv::Content, fabs(value));

    std::shared_ptr<Data> data = std::make_shared<Data>(name);
    data->setContent(content);
    data->setContentType(type);

    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    digest(data);
    return data;
}

// Prepare Data as bytes
std::shared_ptr<Data> Packager::getPackage(ndn::Name &name,
                                           const uint8_t *value, ssize_t size) {
    std::shared_ptr<Data> data = std::make_shared<Data>(name);
    data->setContent(value, size);

    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    digest(data);
    return data;
}
} // namespace xrdndnproducer