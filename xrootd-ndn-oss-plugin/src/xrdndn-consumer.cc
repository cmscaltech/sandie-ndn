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

#include <functional>
#include <iostream>

#include "xrdndn-consumer.hh"
#include "xrdndn-utils.hh"

using namespace ndn;

namespace xrdndn {
Consumer::Consumer()
    : m_scheduler(m_face.getIoService()),
      m_validator(security::v2::getAcceptAllValidator()), m_nTimeouts(0),
      m_nNacks(0), m_retOpen(-1), m_retClose(-1), m_nextToCopy(0),
      m_buffOffset(0) {
    m_bufferedData.clear();
}

Consumer::~Consumer() {
    m_face.removeAllPendingInterests();
    m_scheduler.cancelAllEvents();
    m_face.shutdown();

    m_bufferedData.clear();
}

const Interest Consumer::composeInterest(const Name name) {
    Interest interest(name);
    interest.setInterestLifetime(DEFAULT_INTEREST_LIFETIME);
    interest.setMustBeFresh(true);
    return interest;
}

void Consumer::expressInterest(const Interest &interest,
                               const SystemCalls call) {
    std::function<void(const Interest &interest, const Data &data)> onData;
    switch (call) {
    case (SystemCalls::open):
        onData = bind(&Consumer::onOpenData, this, _1, _2);
        break;
    case (SystemCalls::close):
        onData = bind(&Consumer::onCloseData, this, _1, _2);
        break;
    case (SystemCalls::read):
        onData = bind(&Consumer::onReadData, this, _1, _2);
        break;
    default:
        return;
    }

    m_face.expressInterest(interest, onData,
                           bind(&Consumer::onNack, this, _1, _2, call),
                           bind(&Consumer::onTimeout, this, _1, call));
}

void Consumer::onNack(const Interest &interest, const lp::Nack &nack,
                      const SystemCalls call) {
    if (m_nNacks > MAX_RETRIES) {
        std::cout
            << "xrdndnconsumer: Reached the maximum number of nack retries: "
            << m_nNacks << " while retrieving data for: " << interest
            << std::endl;
        return;
    } else {
        ++m_nNacks;
    }

    Interest newInterest(interest);
    newInterest.refreshNonce();

    switch (nack.getReason()) {
    case lp::NackReason::DUPLICATE:
        this->expressInterest(newInterest, call);
        break;
    case lp::NackReason::CONGESTION:
        m_scheduler.scheduleEvent(
            CONGESTION_TIMEOUT,
            bind(&Consumer::expressInterest, this, interest, call));
        break;
    default:
        std::cout << "xrdndnconsumer: Received NACK with reason: "
                  << nack.getReason() << " for interest " << interest
                  << std::endl;
        break;
    }
}

void Consumer::onTimeout(const Interest &interest, const SystemCalls call) {
    std::cout << "xrdndnconsumer: Timeout for interest: " << interest
              << std::endl;
    if (m_nTimeouts > MAX_RETRIES) {
        std::cout
            << "xrdndnconsumer: Reached the maximum number of timeout retries: "
            << m_nTimeouts << " while retrieving data for: " << interest
            << std::endl;
        return;
    } else {
        ++m_nTimeouts;
    }

    Interest newInterest(interest);
    newInterest.refreshNonce();
    this->expressInterest(newInterest, call);
}

void Consumer::onOpenData(const Interest &interest, const Data &data) {
    m_validator.validate(
        data,
        [this, interest](const Data &data) {
            m_retOpen = this->getIntegerFromData(data);
            std::cout << "xrdndnconsumer: Open file: " << interest
                      << " with error code: " << m_retOpen << std::endl;
        },
        [](const Data &, const security::v2::ValidationError &error) {
            std::cout
                << "xrdndnconsumer: Error while validating data for open: "
                << error.getInfo() << std::endl;
        });
}

int Consumer::Open(std::string path) {
    Interest openInterest =
        this->composeInterest(Utils::interestName(SystemCalls::open, path));
    this->expressInterest(openInterest, SystemCalls::open);

    std::cout << "xrdndnconsumer: Sending open file interest: " << openInterest
              << std::endl;
    m_face.processEvents();

    return m_retOpen;
}

void Consumer::onCloseData(const Interest &interest, const Data &data) {
    m_validator.validate(
        data,
        [this, interest](const Data &data) {
            m_retClose = this->getIntegerFromData(data);
            std::cout << "xrdndnconsumer: Close file: " << interest
                      << " with error code: " << m_retClose << std::endl;
        },
        [](const Data &, const security::v2::ValidationError &error) {
            std::cout
                << "xrdndnconsumer: Error while validating data for close: "
                << error.getInfo() << std::endl;
        });
}

int Consumer::Close(std::string path) {
    Interest openInterest =
        this->composeInterest(Utils::interestName(SystemCalls::close, path));
    this->expressInterest(openInterest, SystemCalls::close);

    std::cout << "xrdndnconsumer: Sending close file interest: " << openInterest
              << std::endl;
    m_face.processEvents();

    return m_retClose;
}

void Consumer::onReadData(const ndn::Interest &interest,
                          const ndn::Data &data) {
    std::cout << "xrdndnconsumer: Read file: Received data for: " << interest
              << std::endl;

    auto dataPtr = data.shared_from_this();
    m_validator.validate(
        data,
        [this, dataPtr](const Data &data) {
            m_bufferedData[data.getName().at(-1).toSegment()] = dataPtr;
        },
        [](const Data &, const security::v2::ValidationError &error) {
            std::cout
                << "xrdndnconsumer: Error while validating data for read: "
                << error.getInfo() << std::endl;
        });
}

ssize_t Consumer::Read(void *buff, off_t offset, size_t blen,
                       std::string path) {

    uint64_t firstSegment = offset / MAX_NDN_PACKET_SIZE;
    uint64_t lastSegment = firstSegment + (blen / MAX_NDN_PACKET_SIZE);

    m_nextToCopy = firstSegment;
    for (auto i = firstSegment; i <= lastSegment; ++i) {
        Name name = Utils::interestName(SystemCalls::read, path);
        name.appendSegment(i); // segment no.

        Interest readInterest = this->composeInterest(name);
        this->expressInterest(readInterest, SystemCalls::read);

        std::cout << "xrdndnconsumer: Sending read file interest: "
                  << readInterest << std::endl;
    }

    m_face.processEvents();
    this->saveDataInOrder(buff, offset, blen);
    return m_buffOffset;
}

void Consumer::saveDataInOrder(void *buff, off_t offset, size_t blen) {
    auto storeInBuff = [&](const Block &content, off_t contentOffset) {
        size_t len = content.value_size() - contentOffset;
        len = len < blen ? len : blen;

        memcpy((uint8_t *)buff + m_buffOffset, content.value() + contentOffset,
               len);
        m_buffOffset += len;
    };

    for (auto it = m_bufferedData.begin();
         it != m_bufferedData.end() && it->first == m_nextToCopy;
         it = m_bufferedData.erase(it), ++m_nextToCopy) {

        const Block &content = it->second->getContent();
        if (m_buffOffset == 0) { // Store first chunk
            off_t contentOffset = offset % MAX_NDN_PACKET_SIZE;
            storeInBuff(content, contentOffset);
        } else {
            storeInBuff(content, 0);
        }
    }
}
} // namespace xrdndn