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

#include <cstdlib>

#include "../common/xrdndn-logger.hh"
#include "../common/xrdndn-namespace.hh"
#include "xrdndn-packager.hh"

using namespace ndn;

namespace xrdndnproducer {
const time::milliseconds Packager::FRESHNESS_PERIOD = time::milliseconds(16000);

const security::SigningInfo Packager::signingInfo =
    static_cast<security::SigningInfo>(
        security::SigningInfo::SignerType::SIGNER_TYPE_SHA256);

const std::shared_ptr<ndn::KeyChain> Packager::keyChain =
    std::make_shared<KeyChain>();

Packager::Packager() {}

Packager::~Packager() {}

void Packager::digest(Data &data) {
    data.setFreshnessPeriod(FRESHNESS_PERIOD);
    keyChain->sign(data, signingInfo);
}

const Data Packager::getPackage(Name &name, const int contentValue) {
    Data data(name.appendVersion());

    data.setContent(
        makeNonNegativeIntegerBlock(ndn::tlv::Content, abs(contentValue)));
    if (contentValue < 0) {
        data.setContentType(tlv::ContentTypeValue::ContentType_Nack);
    }

    digest(data);
    return data;
}

const Data Packager::getPackage(Name &name, const uint8_t *value,
                                ssize_t size) {
    Data data(name.appendVersion());

    data.setContent(value, size);
    digest(data);
    return data;
}
} // namespace xrdndnproducer