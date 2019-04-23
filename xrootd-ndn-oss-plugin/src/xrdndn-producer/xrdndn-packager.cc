/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018-2019 California Institute of Technology                   *
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
const security::SigningInfo Packager::signingInfo =
    static_cast<security::SigningInfo>(
        security::SigningInfo::SignerType::SIGNER_TYPE_SHA256);

const std::shared_ptr<ndn::KeyChain> Packager::keyChain =
    std::make_shared<KeyChain>();

Packager::Packager(uint64_t freshnessPeriod, bool disableSignature)
    : m_freshnessPeriod(freshnessPeriod), m_disableSigning(disableSignature) {
    if (disableSignature) {
        SignatureInfo sigInfo =
            SignatureInfo(static_cast<ndn::tlv::SignatureTypeValue>(255));
        m_fakeSignature.setInfo(sigInfo);
        m_fakeSignature.setValue(makeEmptyBlock(ndn::tlv::SignatureValue));
    }
}

Packager::~Packager() {}

void Packager::digest(std::shared_ptr<ndn::Data> data) {
    data->setFreshnessPeriod(m_freshnessPeriod);

    if (!m_disableSigning) {
        keyChain->sign(*data, signingInfo);
    } else {
        data->setSignature(m_fakeSignature);
    }
}

std::shared_ptr<ndn::Data> Packager::getPackage(ndn::Name &name,
                                                const int contentValue) {
    auto data = std::make_shared<ndn::Data>(name);
    data->setContent(
        makeNonNegativeIntegerBlock(ndn::tlv::Content, abs(contentValue)));
    if (contentValue < 0) {
        data->setContentType(tlv::ContentTypeValue::ContentType_Nack);
    }

    digest(data->shared_from_this());
    return data->shared_from_this();
}

std::shared_ptr<ndn::Data>
Packager::getPackage(ndn::Name &name, const uint8_t *value, ssize_t size) {
    auto data = std::make_shared<ndn::Data>(name);

    data->setContent(value, size);
    digest(data->shared_from_this());
    return data->shared_from_this();
}
} // namespace xrdndnproducer