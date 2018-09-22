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

#include "xrdndn-common.hh"
#include "xrdndn-producer.hh"
#include "xrdndn-utils.hh"

using namespace ndn;

NDN_LOG_INIT(xrdndnproducer);

namespace xrdndn {
Producer::Producer(Face &face)
    : m_face(face), m_scheduler(face.getIoService()), m_OpenFilterId(nullptr),
      m_CloseFilterId(nullptr), m_ReadFilterId(nullptr) {
    NDN_LOG_TRACE("Alloc xrdndn::Producer");
    this->registerPrefix();

    m_dispatcher = std::make_shared<Dispatcher>(&face);
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

    m_scheduler.cancelAllEvents();
    m_face.shutdown();

    // Close all opened files
    NDN_LOG_TRACE("Dealloc xrdndn::Producer. Closing all files.");
    this->m_FileClosingEvets.clear();
    this->m_FileDescriptors.clear();
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

/*****************************************************************************/
/*                                  O p e n                                  */
/*****************************************************************************/
void Producer::onOpenInterest(const InterestFilter &,
                              const Interest &interest) {
    NDN_LOG_TRACE("onOpenInterest: " << interest);

    Name name(interest.getName());
    int ret = this->Open(Utils::getFilePathFromName(name, SystemCalls::open));
    name.appendVersion();

    m_dispatcher->sendInteger(name, ret);
}

int Producer::Open(std::string path) {
    // If the file is already opened, no need to open it again
    if (this->m_FileDescriptors.hasKey(path)) {
        auto closeEventInProgress = this->m_FileClosingEvets.at(path);
        if (closeEventInProgress) {
            NDN_LOG_INFO(
                "File already opened. Cancel closing task in progress.");
            m_scheduler.cancelEvent(closeEventInProgress);
            this->m_FileClosingEvets.erase(path);
        }

        return XRDNDN_ESUCCESS;
    }

    auto fdEntry = std::make_shared<FileDescriptor>(path.c_str());

    if (fdEntry->get() == -1) {
        NDN_LOG_WARN("Failed to open file: " << path);
        return XRDNDN_EFAILURE;
    }

    NDN_LOG_INFO("Opened file: " << path);
    this->m_FileDescriptors.insert(
        std::make_pair<std::string &, std::shared_ptr<FileDescriptor> &>(
            path, fdEntry));

    return XRDNDN_ESUCCESS;
}

/*****************************************************************************/
/*                                 C l o s e                                 */
/*****************************************************************************/
void Producer::onCloseInterest(const InterestFilter &,
                               const Interest &interest) {
    NDN_LOG_TRACE("onCloseInterest: " << interest);

    Name name(interest.getName());
    int ret = this->Close(Utils::getFilePathFromName(name, SystemCalls::close));
    name.appendVersion();

    m_dispatcher->sendInteger(name, ret);
}

int Producer::Close(std::string path) {
    if (!this->m_FileDescriptors.hasKey(path)) {
        NDN_LOG_WARN("File: " << path << " was not oppend previously.");
        return XRDNDN_EFAILURE;
    }

    auto closeEvent =
        m_scheduler.scheduleEvent(CLOSING_FILE_DELAY, [this, path] {
            if (this->m_FileDescriptors.hasKey(path)) {
                NDN_LOG_INFO("Closing file: " << path);
                this->m_FileDescriptors.erase(path);
            }
        });

    this->m_FileClosingEvets.insert(std::make_pair(path, closeEvent));

    NDN_LOG_INFO("Schedule event for closing file: " << path);
    return XRDNDN_ESUCCESS;
}

/*****************************************************************************/
/*                                F s t a t                                  */
/*****************************************************************************/
void Producer::onFstatInterest(const ndn::InterestFilter &,
                               const ndn::Interest &interest) {
    NDN_LOG_TRACE("onFstatInterest: " << interest);

    Name name(interest.getName());
    struct stat info;
    int ret = this->Fstat(&info,
                          Utils::getFilePathFromName(name, SystemCalls::fstat));
    name.appendVersion();

    if (ret != 0) {
        m_dispatcher->sendInteger(name, ret);
    } else {
        // Send stat
        std::string buff(sizeof(info), '\0');
        memcpy(&buff[0], &info, sizeof(info));
        m_dispatcher->sendString(name, buff, sizeof(info));
    }
}

int Producer::Fstat(struct stat *buff, std::string path) {
    if (!this->m_FileDescriptors.hasKey(path)) {
        NDN_LOG_WARN("File: " << path << " was not oppend previously");
        return XRDNDN_EFAILURE;
    }

    // Call stat as is sufficient
    if (stat(path.c_str(), buff) != 0) {
        return XRDNDN_EFAILURE;
    }

    NDN_LOG_INFO("Return fstat for file: " << path);
    return XRDNDN_ESUCCESS;
}

/*****************************************************************************/
/*                                  R e a d                                  */
/*****************************************************************************/
void Producer::onReadInterest(const InterestFilter &,
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
        m_dispatcher->sendInteger(name, count);
    } else {
        m_dispatcher->sendString(name, buff, count);
    }
}

ssize_t Producer::Read(void *buff, off_t offset, size_t blen,
                       std::string path) {

    if (!this->m_FileDescriptors.hasKey(path)) {
        NDN_LOG_WARN("File: " << path << " was not oppend previously");
        if (this->Open(path) == -1)
            return XRDNDN_EFAILURE;
    }

    return pread(this->m_FileDescriptors.at(path)->get(), buff, blen, offset);
}
} // namespace xrdndn