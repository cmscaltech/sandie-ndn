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

#include <algorithm>

#include "../common/xrdndn-logger.hh"
#include "../common/xrdndn-utils.hh"
#include "xrdndn-consumer.hh"

using namespace ndn;

namespace xrdndnconsumer {
const ndn::time::milliseconds Consumer::DEFAULT_INTEREST_LIFETIME =
    ndn::time::seconds(4);

std::shared_ptr<Consumer> Consumer::getXrdNdnConsumerInstance() {
    auto consumer = std::make_shared<Consumer>();

    if (!consumer || consumer->m_error) {
        NDN_LOG_FATAL("Unable to get XRootD NDN Consumer object instance");
        return nullptr;
    }

    return consumer;
}

Consumer::Consumer()
    : m_validator(security::v2::getAcceptAllValidator()), m_error(false) {
    NDN_LOG_TRACE("Alloc XRootD NDN Consumer");

    m_pipeline = std::make_shared<Pipeline>(m_face);
    if (!m_pipeline) {
        m_error = true;
        NDN_LOG_ERROR("Unable to get Pipeline object instance");
    }

    if (!this->processEvents())
        m_error = true;
}

Consumer::~Consumer() {
    m_dataStore.clear();
    m_pipeline->clear();

    m_face.removeAllPendingInterests();
    m_face.shutdown();
}

bool Consumer::processEvents() {
    try {
        m_face.processEvents();
    } catch (const std::exception &e) {
        NDN_LOG_ERROR(e.what());
        return false;
    }
    return true;
}

const Interest Consumer::getInterest(ndn::Name prefix, std::string path,
                                     uint64_t segmentNo,
                                     ndn::time::milliseconds lifetime) {
    auto name = xrdndn::Utils::getName(prefix, path, segmentNo);

    Interest interest(name);
    interest.setInterestLifetime(lifetime);
    interest.setMustBeFresh(true);
    return interest;
}

/*****************************************************************************/
/*                                  O p e n                                  */
/*****************************************************************************/
void Consumer::onOpenData(const Interest &interest, const Data &data) {
    NDN_LOG_TRACE("Received Data for open with Interest: " << interest);
    m_validator.validate(
        data,
        [this](const Data &data) {
            m_FileStat.retOpen = -readNonNegativeInteger(data.getContent());
        },
        [](const Data &, const security::v2::ValidationError &error) {
            NDN_LOG_ERROR(
                "Error while validating Data for open: " << error.getInfo());
        });
}

int Consumer::Open(std::string path) {
    auto openInterest =
        this->getInterest(xrdndn::SYS_CALL_OPEN_PREFIX_URI, path);

    NDN_LOG_INFO("Request open file: " << path
                                       << " with Interest: " << openInterest);

    m_pipeline->insert(openInterest);
    m_pipeline->run(
        std::bind(&Consumer::onOpenData, this, _1, _2),
        [this](const int &errcode) { m_FileStat.retOpen = errcode; });

    if (!this->processEvents()) {
        return -EMSGSIZE;
    }

    NDN_LOG_INFO("Received open Data for file: " << path << " with error code: "
                                                 << m_FileStat.retOpen);
    return m_FileStat.retOpen;
}

/*****************************************************************************/
/*                                 C l o s e                                 */
/*****************************************************************************/
void Consumer::onCloseData(const Interest &interest, const Data &data) {
    NDN_LOG_TRACE("Received Data for close with Interest: " << interest);
    m_validator.validate(
        data,
        [this](const Data &data) {
            m_FileStat.retClose = -readNonNegativeInteger(data.getContent());
        },
        [](const Data &, const security::v2::ValidationError &error) {
            NDN_LOG_ERROR(
                "Error while validating Data for close: " << error.getInfo());
        });
}

int Consumer::Close(std::string path) {
    auto closeInterest =
        this->getInterest(xrdndn::SYS_CALL_CLOSE_PREFIX_URI, path);

    NDN_LOG_INFO("Request close file: " << path
                                        << " with Interest: " << closeInterest);

    m_pipeline->insert(closeInterest);
    m_pipeline->run(
        std::bind(&Consumer::onCloseData, this, _1, _2),
        [this](const int &errcode) { m_FileStat.retClose = errcode; });

    if (!this->processEvents()) {
        return -EMSGSIZE;
    }

    m_pipeline->printStatistics(path);

    NDN_LOG_INFO("Received close Data for file: "
                 << path << " with error code: " << m_FileStat.retClose);
    return m_FileStat.retClose;
}

/*****************************************************************************/
/*                                F s t a t                                  */
/*****************************************************************************/
void Consumer::onFstatData(const ndn::Interest &interest,
                           const ndn::Data &data) {
    NDN_LOG_TRACE("Received Data for fstat with Interest: " << interest);

    m_validator.validate(
        data,
        [this](const Data &data) {
            if (data.getContentType() == ndn::tlv::ContentType_Nack) {
                m_FileStat.retFstat =
                    -readNonNegativeInteger(data.getContent());
            } else {
                m_FileStat.retFstat = XRDNDN_ESUCCESS;
                m_dataStore.insert(
                    std::pair<uint64_t, const Block>(0, data.getContent()));
            }
        },
        [](const Data &, const security::v2::ValidationError &error) {
            NDN_LOG_ERROR(
                "Error while validating Data for fstat: " << error.getInfo());
        });
}

int Consumer::Fstat(struct stat *buff, std::string path) {
    auto fstatInterest =
        this->getInterest(xrdndn::SYS_CALL_FSTAT_PREFIX_URI, path, 0, 16_s);

    NDN_LOG_INFO("Request fstat for file: " << path << " with Interest: "
                                            << fstatInterest);

    m_pipeline->insert(fstatInterest);
    m_pipeline->run(
        std::bind(&Consumer::onFstatData, this, _1, _2),
        [this](const int &errcode) { m_FileStat.retFstat = errcode; });

    if (!this->processEvents()) {
        return -EMSGSIZE;
    }

    if (m_FileStat.retFstat == XRDNDN_ESUCCESS) {
        this->returnData(buff, 0, sizeof(struct stat));
        m_FileStat.fileSize = buff->st_size;
    }

    NDN_LOG_INFO("Received fstat Data for file: "
                 << path << " with error code: " << m_FileStat.retFstat);
    return m_FileStat.retFstat;
}

/*****************************************************************************/
/*                                  R e a d                                  */
/*****************************************************************************/
void Consumer::onReadData(const ndn::Interest &interest,
                          const ndn::Data &data) {
    NDN_LOG_TRACE("Received Data for read with Interest: " << interest);

    m_validator.validate(
        data,
        [this](const Data &data) {
            if (data.getContentType() == ndn::tlv::ContentType_Nack) {
                m_FileStat.retRead = -readNonNegativeInteger(data.getContent());
            } else {
                m_FileStat.retRead = XRDNDN_ESUCCESS;
                m_dataStore.insert(std::pair<uint64_t, const Block>(
                    xrdndn::Utils::getSegmentNo(data.getName()),
                    data.getContent()));
            }
        },
        [](const Data &, const security::v2::ValidationError &error) {
            NDN_LOG_ERROR(
                "Error while validating Data for read: " << error.getInfo());
        });
}

ssize_t Consumer::Read(void *buff, off_t offset, size_t blen,
                       std::string path) {
    // This should be uncommented once we offer multi-threading support
    // and can use one single Consumer instance in XrdNdnFS lib

    // if (offset >= m_FileStat.fileSize)
    //     return XRDNDN_ESUCCESS;

    // if (offset + static_cast<off_t>(blen) > m_FileStat.fileSize)
    //     blen = m_FileStat.fileSize - offset;

    NDN_LOG_TRACE("Reading " << blen << " bytes @" << offset
                             << " from file: " << path);

    off_t startSegment = offset / XRDNDN_MAX_NDN_PACKET_SIZE;
    off_t noSegments = blen / XRDNDN_MAX_NDN_PACKET_SIZE;
    if (noSegments != 0)
        ++noSegments;

    for (off_t i = startSegment; i <= startSegment + noSegments; ++i) {
        m_pipeline->insert(
            this->getInterest(xrdndn::SYS_CALL_READ_PREFIX_URI, path, i));
    }

    m_pipeline->run(
        std::bind(&Consumer::onReadData, this, _1, _2),
        [this](const int &errcode) { m_FileStat.retRead = errcode; });

    if (!this->processEvents()) {
        return -EMSGSIZE;
    }

    if (m_FileStat.retRead == XRDNDN_ESUCCESS) {
        m_FileStat.retRead = this->returnData(buff, offset, blen);
    }

    NDN_LOG_TRACE("Received read Data for "
                  << blen << " bytes @" << offset << " from file: " << path
                  << " with ret: " << m_FileStat.retRead);

    return m_FileStat.retRead;
}

inline size_t Consumer::returnData(void *buff, off_t offset, size_t blen) {
    size_t nBytes = 0;

    auto putData = [&](const Block &content, size_t contentoff) {
        auto len = std::min(content.value_size() - contentoff, blen);
        memcpy((uint8_t *)buff + nBytes, content.value() + contentoff, len);
        nBytes += len;
        blen -= len;
    };

    auto it = m_dataStore.begin();
    // Store first bytes in buffer from offset
    putData(it->second, offset % XRDNDN_MAX_NDN_PACKET_SIZE);
    it = m_dataStore.erase(it);

    // Store rest of bytes until end
    for (; it != m_dataStore.end(); it = m_dataStore.erase(it)) {
        putData(it->second, 0);
    }

    return nBytes;
}
} // namespace xrdndnconsumer