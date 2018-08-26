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

void Consumer::onNack(const Interest &interest, const lp::Nack &nack) {
    std::cout << "xrdndnconsumer: Received NACK with reason: "
              << nack.getReason() << " for interest " << interest << std::endl;
}

void Consumer::onTimeout(const Interest &interest) {
    std::cout << "xrdndnconsumer: Timeout for interest: " << interest
              << std::endl;
}

int Consumer::Open(std::string path) {
    Interest openInterest(Utils::getInterestUri(SystemCalls::open, path));
    openInterest.setInterestLifetime(100_s); // Change this on release
    openInterest.setMustBeFresh(true);

    int ret = -1;
    auto openOnData = [&](const Interest &interest, const Data &data) {
        ret = this->getIntegerFromData(data);
        std::cout << "xrdndnconsumer: Open file: " << path
                  << " with error code: " << ret << std::endl;
    };

    m_face.expressInterest(openInterest, bind(openOnData, _1, _2),
                           bind(&Consumer::onNack, this, _1, _2),
                           bind(&Consumer::onTimeout, this, _1));

    std::cout << "xrdndnconsumer: Sending open file interest: " << openInterest
              << std::endl;
    m_face.processEvents();

    return ret;
}

int Consumer::Close(std::string path) {
    Interest openInterest(Utils::getInterestUri(SystemCalls::close, path));
    openInterest.setInterestLifetime(100_s); // Change this on release
    openInterest.setMustBeFresh(true);

    int ret = -1;
    auto closeOnData = [&](const Interest &interest, const Data &data) {
        ret = this->getIntegerFromData(data);
        std::cout << "xrdndnconsumer: Close file: " << path
                  << " with error code: " << ret << std::endl;
    };

    m_face.expressInterest(openInterest, bind(closeOnData, _1, _2),
                           bind(&Consumer::onNack, this, _1, _2),
                           bind(&Consumer::onTimeout, this, _1));

    std::cout << "xrdndnconsumer: Sending close file interest: " << openInterest
              << std::endl;
    m_face.processEvents();

    return ret;
}

ssize_t Consumer::Read(void *buff, off_t offset, size_t blen,
                       std::string path) {

    auto fileReader = shared_ptr<FileReader>(new FileReader(m_face));
    return fileReader->Read(buff, offset, blen, path);
}

int Consumer::getIntegerFromData(const Data &data) {
    int value = readNonNegativeInteger(data.getContent());
    return data.getContentType() == xrdndn::tlv::negativeInteger ? -value
                                                                 : value;
}

FileReader::FileReader(Face &face) : m_face(face) {
    m_bufferedData.clear();
    m_nextToCopy = 0;
    m_buffOffset = 0;
}

FileReader::~FileReader() { m_bufferedData.clear(); }

void FileReader::onNack(const Interest &interest, const lp::Nack &nack) {
    std::cout << "xrdndnconsumer: Received NACK with reason: "
              << nack.getReason() << " for interest " << interest << std::endl;

    // TODO
}

void FileReader::onTimeout(const Interest &interest) {
    std::cout << "xrdndnconsumer: Timeout for interest: " << interest
              << std::endl;

    // TODO
}

ssize_t FileReader::Read(void *buff, off_t offset, size_t blen,
                         std::string path) {

    auto onData = [&](const Interest &interest, const Data &data) {
        std::cout << "xrdndnconsumer: Read file: Received data for: "
                  << interest << std::endl;
        m_bufferedData[0] = data.shared_from_this();
        this->saveDataInOrder(buff, offset, blen);
    };

    uint64_t firstSegment = offset / MAX_NDN_PACKET_SIZE;
    uint64_t lastSegment = firstSegment + (blen / MAX_NDN_PACKET_SIZE);

    m_nextToCopy = firstSegment;
    for (auto i = firstSegment; i <= lastSegment; ++i) {
        Name name = Utils::getInterestUri(SystemCalls::read, path);
        name.appendSegment(i); // segment no.

        Interest interest(name);
        interest.setInterestLifetime(100_s);
        interest.setMustBeFresh(true);

        std::cout << "xrdndnconsumer: Sending read file interest: " << interest
                  << std::endl;

        m_face.expressInterest(interest, bind(onData, _1, _2),
                               bind(&FileReader::onNack, this, _1, _2),
                               bind(&FileReader::onTimeout, this, _1));
    }

    m_face.processEvents();

    return m_buffOffset;
}

void FileReader::saveDataInOrder(void *buff, off_t offset, size_t blen) {
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