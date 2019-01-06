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

#include <functional>
#include <iostream>

#include "../common/xrdndn-logger.hh"
#include "../common/xrdndn-utils.hh"
#include "xrdndn-consumer.hh"

using namespace ndn;

namespace xrdndnconsumer {
Consumer::Consumer()
    : m_scheduler(m_face.getIoService()),
      m_validator(security::v2::getAcceptAllValidator()), m_nTimeouts(0),
      m_nNacks(0), m_buffOffset(XRDNDN_ESUCCESS), m_retOpen(XRDNDN_EFAILURE),
      m_retClose(XRDNDN_EFAILURE), m_retFstat(XRDNDN_EFAILURE),
      m_retRead(XRDNDN_ESUCCESS) {
    m_bufferedData.clear();
}

Consumer::~Consumer() {
    m_face.removeAllPendingInterests();
    m_scheduler.cancelAllEvents();
    m_face.shutdown();

    this->flush();
}

// Reset all class members.
void Consumer::flush() {
    m_bufferedData.clear();
    m_retOpen = XRDNDN_EFAILURE;
    m_retClose = XRDNDN_EFAILURE;
    m_retFstat = XRDNDN_EFAILURE;
    m_retRead = XRDNDN_ESUCCESS;
    m_nTimeouts = 0;
    m_nNacks = 0;
    m_buffOffset = XRDNDN_ESUCCESS;
}

// Return an interest for the given name.
const Interest Consumer::composeInterest(const Name name) {
    Interest interest(name);
    interest.setInterestLifetime(16_s);
    interest.setMustBeFresh(true);
    return interest;
}

// Express interest on face for a given system call.
void Consumer::expressInterest(const Interest &interest,
                               const xrdndn::SystemCalls call) {
    std::function<void(const Interest &interest, const Data &data)> onData;
    switch (call) {
    case (xrdndn::SystemCalls::open):
        onData = bind(&Consumer::onOpenData, this, _1, _2);
        break;
    case (xrdndn::SystemCalls::close):
        onData = bind(&Consumer::onCloseData, this, _1, _2);
        break;
    case (xrdndn::SystemCalls::read):
        onData = bind(&Consumer::onReadData, this, _1, _2);
        break;
    case (xrdndn::SystemCalls::fstat):
        onData = bind(&Consumer::onFstatData, this, _1, _2);
        break;
    default:
        return;
    }

    m_face.expressInterest(interest, onData,
                           bind(&Consumer::onNack, this, _1, _2, call),
                           bind(&Consumer::onTimeout, this, _1, call));
}

// Process any data to receive and exit gracefully
int Consumer::processEvents() {
    try {
        m_face.processEvents();
    } catch (const std::exception &e) {
        NDN_LOG_ERROR(e.what());
        return XRDNDN_EFAILURE;
    }
    return XRDNDN_ESUCCESS;
}

// On NACK, the interest will be send again for MAX_ENTRIES times o DUPLICATE
// or CONGESTION reasons.
void Consumer::onNack(const Interest &interest, const lp::Nack &nack,
                      const xrdndn::SystemCalls call) {
    NDN_LOG_TRACE("NACK with reason: " << nack.getReason() << " for interest "
                                       << interest);
    if (m_nNacks >= MAX_RETRIES) {
        NDN_LOG_WARN("Reached the maximum number of nack retries: "
                     << m_nNacks << " while retrieving Data for: " << interest);
        return;
    } else {
        ++m_nNacks;
    }

    Interest newInterest(interest);
    newInterest.refreshNonce();

    switch (nack.getReason()) {
    case lp::NackReason::DUPLICATE:
        NDN_LOG_TRACE("Resending interest: " << interest);
        m_scheduler.scheduleEvent(
            DUPLICATE_BACKOFF,
            bind(&Consumer::expressInterest, this, interest, call));
        break;
    case lp::NackReason::CONGESTION:
        NDN_LOG_TRACE("Resending interest: " << interest);
        m_scheduler.scheduleEvent(
            CONGESTION_BACKOFF,
            bind(&Consumer::expressInterest, this, interest, call));
        break;
    default:
        NDN_LOG_WARN("NACK with reason: "
                     << nack.getReason()
                     << " does not trigger a retry for interest: " << interest);
        break;
    }
}

// On timeout, retry for MAX_RETRIES times
void Consumer::onTimeout(const Interest &interest,
                         const xrdndn::SystemCalls call) {
    NDN_LOG_TRACE("Timeout for interest: " << interest);

    if (m_nTimeouts >= MAX_RETRIES) {
        NDN_LOG_WARN("Reached the maximum number of timeout retries: "
                     << m_nTimeouts
                     << " while retrieving Data for: " << interest);
        return;
    } else {
        ++m_nTimeouts;
    }

    Interest newInterest(interest);
    newInterest.refreshNonce();

    NDN_LOG_TRACE("Resending interest: " << interest);
    m_scheduler.scheduleEvent(TIMEOUT_BACKOFF, bind(&Consumer::expressInterest,
                                                    this, interest, call));
}

/*****************************************************************************/
/*                                  O p e n                                  */
/*****************************************************************************/
void Consumer::onOpenData(const Interest &interest, const Data &data) {
    m_validator.validate(
        data,
        [this, interest](const Data &data) {
            m_retOpen = this->getIntegerFromData(data);
            NDN_LOG_TRACE("Open file: " << interest
                                        << " with error code: " << m_retOpen);
        },
        [](const Data &, const security::v2::ValidationError &error) {
            NDN_LOG_ERROR(
                "Error while validating Data for open: " << error.getInfo());
        });
}

int Consumer::Open(std::string path) {
    Interest openInterest = this->composeInterest(
        xrdndn::Utils::interestName(xrdndn::SystemCalls::open, path));
    this->expressInterest(openInterest, xrdndn::SystemCalls::open);

    NDN_LOG_TRACE("Sending open file interest: " << openInterest);
    if (this->processEvents()) {
        return XRDNDN_EFAILURE;
    }

    return m_retOpen;
}

/*****************************************************************************/
/*                                 C l o s e                                 */
/*****************************************************************************/
void Consumer::onCloseData(const Interest &interest, const Data &data) {
    m_validator.validate(
        data,
        [this, interest](const Data &data) {
            m_retClose = this->getIntegerFromData(data);
            NDN_LOG_TRACE("Close file: " << interest
                                         << " with error code: " << m_retClose);
        },
        [](const Data &, const security::v2::ValidationError &error) {
            NDN_LOG_ERROR(
                "Error while validating Data for close: " << error.getInfo());
        });
}

int Consumer::Close(std::string path) {
    Interest openInterest = this->composeInterest(
        xrdndn::Utils::interestName(xrdndn::SystemCalls::close, path));
    this->expressInterest(openInterest, xrdndn::SystemCalls::close);

    NDN_LOG_TRACE("Sending close file interest: " << openInterest);
    if (this->processEvents()) {
        return XRDNDN_EFAILURE;
    }

    return m_retClose;
}

/*****************************************************************************/
/*                                F s t a t                                  */
/*****************************************************************************/
void Consumer::onFstatData(const ndn::Interest &interest,
                           const ndn::Data &data) {
    NDN_LOG_TRACE("Received data for fstat: " << interest);

    auto dataPtr = data.shared_from_this();
    m_validator.validate(
        data,
        [this, dataPtr, interest](const Data &data) {
            if (data.getContentType() == xrdndn::tlv::negativeInteger) {
                m_face.shutdown();
            } else {
                m_retFstat = XRDNDN_ESUCCESS;
                m_bufferedData[0] = dataPtr;
            }
        },
        [](const Data &, const security::v2::ValidationError &error) {
            NDN_LOG_ERROR(
                "Error while validating Data for fstat: " << error.getInfo());
        });
}

int Consumer::Fstat(struct stat *buff, std::string path) {
    this->flush();

    Interest fstatInterest = this->composeInterest(
        xrdndn::Utils::interestName(xrdndn::SystemCalls::fstat, path));
    this->expressInterest(fstatInterest, xrdndn::SystemCalls::fstat);

    NDN_LOG_TRACE("Sending fstat interest: " << fstatInterest);
    if (this->processEvents()) {
        return XRDNDN_EFAILURE;
    }

    this->saveDataInOrder(buff, 0, sizeof(struct stat));
    return m_retFstat;
}

/*****************************************************************************/
/*                                  R e a d                                  */
/*****************************************************************************/
void Consumer::onReadData(const ndn::Interest &interest,
                          const ndn::Data &data) {
    NDN_LOG_TRACE("Received data for read: " << interest);
    auto dataPtr = data.shared_from_this();
    m_validator.validate(
        data,
        [this, dataPtr](const Data &data) {
            if (data.getContentType() == xrdndn::tlv::negativeInteger) {
                m_retRead = XRDNDN_EFAILURE;
                m_face.shutdown();
            } else {
                m_bufferedData[xrdndn::Utils::getSegmentFromPacket(data)] =
                    dataPtr;
            }
        },
        [](const Data &, const security::v2::ValidationError &error) {
            NDN_LOG_ERROR(
                "Error while validating Data for read: " << error.getInfo());
        });
}

ssize_t Consumer::Read(void *buff, off_t offset, size_t blen,
                       std::string path) {
    this->flush();
    uint64_t firstSegment = offset / XRDNDN_MAX_NDN_PACKET_SIZE;

    int noSegments = blen / XRDNDN_MAX_NDN_PACKET_SIZE;
    uint64_t lastSegment = firstSegment + noSegments;
    if (noSegments != 0)
        ++lastSegment;

    for (auto i = firstSegment; i <= lastSegment; ++i) {
        Name name =
            xrdndn::Utils::interestName(xrdndn::SystemCalls::read, path);
        name.appendSegment(i); // segment no.

        Interest readInterest = this->composeInterest(name);
        this->expressInterest(readInterest, xrdndn::SystemCalls::read);

        NDN_LOG_TRACE("Sending read file interest: " << readInterest);
    }

    if (this->processEvents()) {
        return XRDNDN_EFAILURE;
    }

    this->saveDataInOrder(buff, offset, blen);
    return m_retRead == XRDNDN_ESUCCESS ? m_buffOffset : XRDNDN_EFAILURE;
}

// Store data received from producer.
void Consumer::saveDataInOrder(void *buff, off_t offset, size_t blen) {
    size_t leftToSave = blen;
    auto storeInBuff = [&](const Block &content, off_t contentOffset) {
        size_t len = content.value_size() - contentOffset;
        len = len < leftToSave ? len : leftToSave;
        leftToSave -= len;

        memcpy((uint8_t *)buff + m_buffOffset, content.value() + contentOffset,
               len);
        m_buffOffset += len;
    };

    for (auto it = m_bufferedData.begin(); it != m_bufferedData.end();
         it = m_bufferedData.erase(it)) {
        const Block &content = it->second->getContent();

        if (m_buffOffset == 0) { // Store first chunk
            size_t contentOffset = offset % XRDNDN_MAX_NDN_PACKET_SIZE;
            if (contentOffset >= content.value_size())
                return;
            storeInBuff(content, contentOffset);
        } else {
            storeInBuff(content, 0);
        }
    }
}
} // namespace xrdndnconsumer