/**************************************************************************
 * Named Data Networking plugin for xrootd                                *
 * Copyright © 2018 California Institute of Technology                    *
 *                                                                        *
 * Author: Catalin Iordache <catalin.iordache@cern.ch>                    *
 *                                                                        *
 * This program is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>. *
 **************************************************************************/

#include <iostream>

#include "xrdndn-common.hh"
#include "xrdndn-consumer.hh"
#include "xrdndn-utils.hh"

using namespace ndn;

namespace xrdndn {
Consumer::Consumer() {}

Consumer::~Consumer() { m_face.shutdown(); }

int Consumer::Open(std::string path) {
    Interest openInterest(Utils::getInterestUri(SystemCalls::open, path));
    openInterest.setInterestLifetime(30_s); // Change this on release
    openInterest.setMustBeFresh(true);

    int ret = -1;
    auto openOnData = [&](const Interest &interest, const Data &data) {
        ret = this->getIntegerFromData(data);
        std::cout << "xrdndnconsumer: Open file: " << path
                  << " with error code: " << ret << std::endl;
    };

    auto openOnNack = [](const Interest &interest, const lp::Nack &nack) {
        std::cout << "xrdndnconsumer: Received NACK with reason: "
                  << nack.getReason() << " for interest " << interest
                  << std::endl;
    };

    auto openOnTimeout = [](const Interest &interest) {
        std::cout << "xrdndnconsumer: Timeout for interest: " << interest
                  << std::endl;
    };

    m_face.expressInterest(openInterest, bind(openOnData, _1, _2),
                           bind(openOnNack, _1, _2), bind(openOnTimeout, _1));

    std::cout << "xrdndnconsumer: Sending open file interest: " << openInterest
              << std::endl;
    m_face.processEvents();

    return ret;
}

int Consumer::getIntegerFromData(const Data &data) {
    int value = readNonNegativeInteger(data.getContent());
    return data.getContentType() == xrdndn::tlv::negativeInteger ? -value
                                                                 : value;
}
} // namespace xrdndn