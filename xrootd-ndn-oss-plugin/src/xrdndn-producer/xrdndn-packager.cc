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
Packager::Packager(uint32_t signerType) {
    m_signerType =
        static_cast<ndn::security::SigningInfo::SignerType>(signerType);
}
Packager::~Packager() {}

void Packager::digest(std::shared_ptr<ndn::Data> &data) {
    data->setFreshnessPeriod(DEFAULT_FRESHNESS_PERIOD);
    security::SigningInfo signingInfo(m_signerType);
    m_keyChain.sign(*data, signingInfo);
}

// Prepare Data containing an non/negative integer
std::shared_ptr<Data> Packager::getPackage(const ndn::Name &name, int value) {
    const Block content =
        makeNonNegativeIntegerBlock(ndn::tlv::Content, fabs(value));

    std::shared_ptr<Data> data = std::make_shared<Data>(name);
    data->setContent(content);

    if (value < 0) {
        data->setContentType(ndn::tlv::ContentType_Nack);
    }

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