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

#include <iostream>

#include "../common/xrdndn-common.hh"
#include "../common/xrdndn-logger.hh"
#include "../common/xrdndn-utils.hh"
#include "xrdndn-producer.hh"

using namespace ndn;
using namespace xrdndn;

namespace xrdndnproducer {

Producer::Producer(Face &face, const Options &opts)
    : m_face(face), m_options(opts), m_OpenFilterId(nullptr),
      m_CloseFilterId(nullptr), m_ReadFilterId(nullptr) {

    NDN_LOG_TRACE("Alloc xrdndn::Producer");
    this->registerPrefix();
    m_packager = std::make_shared<Packager>(opts.signerType);
    m_GarbageCollectorTimer =
        std::make_shared<SystemTimer>(m_face.getIoService());
    m_GarbageCollectorTimer->async_wait(
        std::bind(&Producer::onGarbageCollector, this));
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
    if (m_FstatFilterId != nullptr) {
        m_face.unsetInterestFilter(m_FstatFilterId);
    }
    if (m_ReadFilterId != nullptr) {
        m_face.unsetInterestFilter(m_ReadFilterId);
    }

    m_GarbageCollectorTimer->cancel();
    m_face.shutdown();
    m_FileHandlers.clear();
}

// Register all interest filters that this producer will answer to
void Producer::registerPrefix() {
    NDN_LOG_TRACE("Register prefixes.");

    // For nfd
    m_xrdndnPrefixId = m_face.registerPrefix(
        Name(PLUGIN_INTEREST_PREFIX_URI),
        [](const Name &name) {
            NDN_LOG_INFO("Successfully registered prefix for: " << name);
        },
        [](const Name &name, const std::string &msg) {
            NDN_LOG_FATAL("Could not register " << name
                                                << " prefix for nfd: " << msg);
        });

    // Filter for open system call
    m_OpenFilterId =
        m_face.setInterestFilter(Utils::interestPrefix(SystemCalls::open),
                                 bind(&Producer::onOpenInterest, this, _1, _2));
    if (!m_OpenFilterId) {
        NDN_LOG_FATAL("Could not set interest filter for open systemcall.");
    } else {
        NDN_LOG_INFO("Successfully registered prefix for: "
                     << Utils::interestPrefix(SystemCalls::open));
    }

    // Filter for close system call
    m_CloseFilterId = m_face.setInterestFilter(
        Utils::interestPrefix(SystemCalls::close),
        bind(&Producer::onCloseInterest, this, _1, _2));
    if (!m_CloseFilterId) {
        NDN_LOG_FATAL("Could not set interest filter for close systemcall.");
    } else {
        NDN_LOG_INFO("Successfully registered prefix for: "
                     << Utils::interestPrefix(SystemCalls::close));
    }

    // Filter for fstat system call
    m_FstatFilterId = m_face.setInterestFilter(
        Utils::interestPrefix(SystemCalls::fstat),
        bind(&Producer::onFstatInterest, this, _1, _2));
    if (!m_FstatFilterId) {
        NDN_LOG_FATAL("Could not set interest filter for fstat systemcall.");
    } else {
        NDN_LOG_INFO("Successfully registered prefix for: "
                     << Utils::interestPrefix(SystemCalls::fstat));
    }

    // Filter for read system call
    m_ReadFilterId =
        m_face.setInterestFilter(Utils::interestPrefix(SystemCalls::read),
                                 bind(&Producer::onReadInterest, this, _1, _2));
    if (!m_CloseFilterId) {
        NDN_LOG_FATAL("Could not set interest filter for read systemcall.");
    } else {
        NDN_LOG_INFO("Successfully registered prefix for: "
                     << Utils::interestPrefix(SystemCalls::read));
    }
}

void Producer::onGarbageCollector() {
    boost::unique_lock<boost::shared_mutex>(m_FileHandlers.mutex_);
    auto it = m_FileHandlers.begin();
    while (it != m_FileHandlers.end()) {
        if (it->second->refCount == 0) {
            NDN_LOG_INFO("Dealloc FileHandler object for file: " << it->first);
            it = m_FileHandlers.erase(it);
        } else {
            ++it;
        }
    }

    m_GarbageCollectorTimer->expires_from_now(std::chrono::seconds(90));
    m_GarbageCollectorTimer->async_wait(
        std::bind(&Producer::onGarbageCollector, this));
}

bool Producer::setFileHandler(std::string path) {
    if (m_FileHandlers.hasKey(path)) {
        return true;
    }

    auto entry = std::make_shared<FileHandler>(m_options);
    auto ret = m_FileHandlers.insert(
        std::make_pair<std::string &, std::shared_ptr<FileHandler> &>(path,
                                                                      entry));

    if (!ret.second) {
        NDN_LOG_ERROR("Failed to create file handler for: " << path);
        m_FileHandlers.erase(path);
        return false;
    }
    return true;
}

void Producer::onOpenInterest(const InterestFilter &,
                              const Interest &interest) {
    m_face.getIoService().post([&] {
        NDN_LOG_TRACE("onOpenInterest: " << interest);

        Name name(interest.getName());
        std::string path =
            xrdndn::Utils::getFilePathFromName(name, xrdndn::SystemCalls::open);

        std::shared_ptr<Data> data;
        if (!setFileHandler(path)) {
            data = m_packager->getPackage(name, XRDNDN_EFAILURE);
        } else {
            data = m_FileHandlers.at(path)->getOpenData(name, path);
        }

        NDN_LOG_TRACE("Sending: " << *data);
        m_face.put(*data);
    });
}

void Producer::onCloseInterest(const InterestFilter &,
                               const Interest &interest) {
    m_face.getIoService().post([&] {
        NDN_LOG_TRACE("onCloseInterest: " << interest);
        Name name(interest.getName());
        std::string path = xrdndn::Utils::getFilePathFromName(
            name, xrdndn::SystemCalls::close);

        std::shared_ptr<Data> data;
        if (m_FileHandlers.hasKey(path)) {
            data = m_FileHandlers.at(path)->getCloseData(name, path);
        } else {
            data = m_packager->getPackage(name, XRDNDN_ESUCCESS);
        }

        NDN_LOG_TRACE("Sending: " << *data);
        m_face.put(*data);
    });
}

void Producer::onFstatInterest(const ndn::InterestFilter &,
                               const ndn::Interest &interest) {
    m_face.getIoService().post([&] {
        NDN_LOG_TRACE("onFstatInterest: " << interest);
        Name name(interest.getName());
        std::string path = xrdndn::Utils::getFilePathFromName(
            name, xrdndn::SystemCalls::fstat);

        std::shared_ptr<Data> data;
        if (!setFileHandler(path)) {
            data = m_packager->getPackage(name, XRDNDN_EFAILURE);
        } else {
            data = m_FileHandlers.at(path)->getFStatData(name, path);
        }

        NDN_LOG_TRACE("Sending: " << *data);
        m_face.put(*data);
    });
}

void Producer::onReadInterest(const InterestFilter &,
                              const Interest &interest) {
    m_face.getIoService().post([&] {
        NDN_LOG_TRACE("onReadInterest: " << interest);
        Name name(interest.getName());
        std::string path =
            xrdndn::Utils::getFilePathFromName(name, xrdndn::SystemCalls::read);

        std::shared_ptr<Data> data;
        if (!setFileHandler(path)) {
            data = m_packager->getPackage(name, XRDNDN_EFAILURE);
        } else {
            data = m_FileHandlers.at(path)->getReadData(
                xrdndn::Utils::getSegmentFromPacket(interest), name, path);
        }

        NDN_LOG_TRACE("Sending: " << *data);
        m_face.put(*data);
    });
}
} // namespace xrdndnproducer