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
#include <math.h>

#include "xrdndn-common.hh"
#include "xrdndn-producer.hh"
#include "xrdndn-utils.hh"

using namespace ndn;

namespace xrdndn {
Producer::Producer(Face &face) : m_face(face), m_OpenFileFilterId(nullptr) {
    this->registerPrefix();
}

Producer::~Producer() {
    if (m_xrdndnPrefixId != nullptr) {
        m_face.unsetInterestFilter(m_xrdndnPrefixId);
    }
    if (m_OpenFileFilterId != nullptr) {
        m_face.unsetInterestFilter(m_OpenFileFilterId);
    }

    // Close all opened files
    boost::shared_lock<boost::shared_mutex> lock(
        this->m_FileDescriptors.mutex_);
    for (auto it = this->m_FileDescriptors.begin();
         it != this->m_FileDescriptors.end(); ++it) {
        it->second->close();
    }
    lock.unlock();
    this->m_FileDescriptors.clear();
}

void Producer::registerPrefix() {
    // For nfd
    m_xrdndnPrefixId = m_face.registerPrefix(
        Name(PLUGIN_INTEREST_PREFIX_URI), [](const Name &name) {},
        [](const Name &name, const std::string &msg) {
            // throw Error("cannot register ndnxrd prefix.");
            std::cout << "xrdndnproducer: Cannot register ndnxrd prefix: "
                      << msg << std::endl;
        });

    // Filter for file open system call
    m_OpenFileFilterId =
        m_face.setInterestFilter(Utils::getInterestPrefix(SystemCalls::open),
                                 bind(&Producer::onOpenInterest, this, _1, _2));
}

void Producer::send(const ndn::Name &name, const Block &content,
                    uint32_t type) {
    shared_ptr<Data> data = make_shared<Data>(name);
    data->setFreshnessPeriod(60_s);
    data->setContent(content);
    data->setContentType(type);

    m_keyChain.sign(*data); // signWithDigestSha256

    std::cout << "xrdndnproducer: D: " << *data << std::endl;
    m_face.put(*data);
}

void Producer::sendInteger(const ndn::Name &name, int value) {
    int type = value < 0 ? xrdndn::tlv::negativeInteger
                         : xrdndn::tlv::nonNegativeInteger;
    const Block content =
        ndn::makeNonNegativeIntegerBlock(ndn::tlv::Content, fabs(value));
    this->send(name, content, type);
}

void Producer::onOpenInterest(const InterestFilter &filter,
                              const Interest &interest) {
    std::cout << "xrdndnproducer I: " << interest << std::endl;
    std::cout << "xrdndnproducer: Filter: " << filter << std::endl;
    Name name(interest.getName());

    int ret = Open(Utils::getFilePathFromName(name, SystemCalls::open));

    name.appendVersion();
    this->sendInteger(name, ret);
}

int Producer::Open(std::string path) {
    std::ifstream fstream;
    fstream.open(path, std::ifstream::in);
    if (fstream.is_open()) {
        this->m_FileDescriptors.insert(
            std::make_pair<std::string, std::ifstream *>(std::string(path),
                                                         &fstream));
        return 0;
    } else {
        std::cout << "xrdndnproducer: Failed to open file: " << path
                  << std::endl;
    }

    return -1;
}
} // namespace xrdndn