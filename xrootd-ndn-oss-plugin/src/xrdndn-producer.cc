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

NDN_LOG_INIT(xrdndnproducer);

namespace xrdndn {
Producer::Producer(Face &face)
    : m_face(face), m_OpenFilterId(nullptr), m_CloseFilterId(nullptr),
      m_ReadFilterId(nullptr) {
    NDN_LOG_TRACE("Alloc xrdndn::Producer");
    this->registerPrefix();
}

Producer::~Producer() {
    if (m_xrdndnPrefixId != nullptr) {
        m_face.unsetInterestFilter(m_xrdndnPrefixId);
    }
    if (m_OpenFilterId != nullptr) {
        m_face.unsetInterestFilter(m_OpenFilterId);
    }
    if (m_CloseFilterId != nullptr) {
        m_face.unsetInterestFilter(m_CloseFilterId);
    }
    if (m_ReadFilterId != nullptr) {
        m_face.unsetInterestFilter(m_ReadFilterId);
    }
    m_face.shutdown();

    // Close all opened files
    NDN_LOG_TRACE("Dealloc xrdndn::Producer. Closing all files.");
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
    NDN_LOG_TRACE("Register prefixes.");

    // For nfd
    m_xrdndnPrefixId = m_face.registerPrefix(
        Name(PLUGIN_INTEREST_PREFIX_URI), [](const Name &name) {},
        [](const Name &name, const std::string &msg) {
            NDN_LOG_FATAL("Could not register " << PLUGIN_INTEREST_PREFIX_URI
                                                << " prefix for nfd: " << msg);
        });

    // Filter for open system call
    m_OpenFilterId =
        m_face.setInterestFilter(Utils::interestPrefix(SystemCalls::open),
                                 bind(&Producer::onOpenInterest, this, _1, _2));
    if (!m_OpenFilterId) {
        NDN_LOG_FATAL("Could not set interest filter for open systemcall.");
    }

    // Filter for close system call
    m_CloseFilterId = m_face.setInterestFilter(
        Utils::interestPrefix(SystemCalls::close),
        bind(&Producer::onCloseInterest, this, _1, _2));
    if (!m_CloseFilterId) {
        NDN_LOG_FATAL("Could not set interest filter for close systemcall.");
    }

    // Filter for read system call
    m_ReadFilterId =
        m_face.setInterestFilter(Utils::interestPrefix(SystemCalls::read),
                                 bind(&Producer::onReadInterest, this, _1, _2));
    if (!m_CloseFilterId) {
        NDN_LOG_FATAL("Could not set interest filter for read systemcall.");
    }
}

void Producer::send(std::shared_ptr<Data> data) {
    data->setFreshnessPeriod(DEFAULT_FRESHNESS_PERIOD);
    m_keyChain.sign(*data); // signWithDigestSha256

    NDN_LOG_INFO("Sending: " << *data);
    m_face.put(*data);
}

void Producer::sendInteger(const Name &name, int value) {
    NDN_LOG_TRACE("Sending integer.");

    int type = value < 0 ? xrdndn::tlv::negativeInteger
                         : xrdndn::tlv::nonNegativeInteger;
    const Block content =
        makeNonNegativeIntegerBlock(ndn::tlv::Content, fabs(value));

    std::shared_ptr<ndn::Data> data = std::make_shared<Data>(name);
    data->setContent(content);
    data->setContentType(type);
    this->send(data);
}

void Producer::sendString(const Name &name, std::string buff, ssize_t size) {
    NDN_LOG_TRACE("Sending string.");

    std::shared_ptr<ndn::Data> data = std::make_shared<Data>(name);
    data->setContent(reinterpret_cast<const uint8_t *>(buff.data()), size);
    this->send(data);
}

void Producer::onOpenInterest(const InterestFilter &filter,
                              const Interest &interest) {
    NDN_LOG_TRACE("onOpenInterest: " << interest);

    Name name(interest.getName());
    int ret = this->Open(Utils::getFilePathFromName(name, SystemCalls::open));
    name.appendVersion();

    this->sendInteger(name, ret);
}

int Producer::Open(std::string path) {
    std::shared_ptr<std::ifstream> fstream = std::make_shared<std::ifstream>();
    fstream->open(path, std::ifstream::in);
    if (fstream->is_open()) {
        NDN_LOG_INFO("Opened file: " << path);
        this->m_FileDescriptors.insert(
            std::make_pair<std::string &, std::shared_ptr<std::ifstream> &>(
                path, fstream));

        return XRDNDN_ESUCCESS;
    } else {
        NDN_LOG_WARN("Failed to open file: " << path);
    }
    return XRDNDN_EFAILURE;
}

void Producer::onCloseInterest(const InterestFilter &filter,
                               const Interest &interest) {
    NDN_LOG_TRACE("onCloseInterest: " << interest);

    Name name(interest.getName());
    int ret = this->Close(Utils::getFilePathFromName(name, SystemCalls::close));
    name.appendVersion();

    this->sendInteger(name, ret);
}

int Producer::Close(std::string path) {
    if (!this->m_FileDescriptors.hasKey(path)) {
        NDN_LOG_WARN("File: " << path << " was not oppend previously.");
        return XRDNDN_EFAILURE;
    }

    auto fstream = this->m_FileDescriptors.at(path);
    boost::unique_lock<boost::shared_mutex> lock(
        this->m_FileDescriptors.mutex_);
    fstream->close();
    lock.unlock();

    if (fstream->is_open()) {
        NDN_LOG_WARN("Failed to close file: " << path);
        return XRDNDN_EFAILURE;
    }

    this->m_FileDescriptors.erase(path);
    return XRDNDN_ESUCCESS;
}

void Producer::onReadInterest(const InterestFilter &filter,
                              const Interest &interest) {
    NDN_LOG_TRACE("onReadInterest: " << interest);

    Name name(interest.getName());

    std::string buff(XRDNDN_MAX_NDN_PACKET_SIZE, '\0');
    uint64_t segmentNo = Utils::getSegmentFromPacket(interest);

    int count = this->Read(&buff[0], segmentNo * XRDNDN_MAX_NDN_PACKET_SIZE,
                           XRDNDN_MAX_NDN_PACKET_SIZE,
                           Utils::getFilePathFromName(name, SystemCalls::read));

    name.appendVersion();
    if (count == XRDNDN_EFAILURE) {
        this->sendInteger(name, count);
    } else {
        this->sendString(name, buff, count);
    }
}

ssize_t Producer::Read(void *buff, off_t offset, size_t blen,
                       std::string path) {

    if (!this->m_FileDescriptors.hasKey(path)) {
        NDN_LOG_WARN("File: " << path << " was not oppend previously");
        return XRDNDN_EFAILURE;
    }

    auto fstream = this->m_FileDescriptors.at(path);
    boost::unique_lock<boost::shared_mutex> lock(
        this->m_FileDescriptors.mutex_);
    fstream->seekg(offset, fstream->beg);
    fstream->read((char *)buff, blen);
    return fstream->gcount();
}

} // namespace xrdndn