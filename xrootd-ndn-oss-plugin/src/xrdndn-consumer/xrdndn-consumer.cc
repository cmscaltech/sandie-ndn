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

    processEvents(false);

    if (!m_error) {
        faceProcessEventsThread =
            boost::thread(std::bind(&Consumer::processEvents, this, true));
    }
}

Consumer::~Consumer() {
    m_pipeline->stop();
    m_face.removeAllPendingInterests();
    m_face.shutdown();
    faceProcessEventsThread.join();
}

void Consumer::processEvents(bool keepThread) {
    try {
        m_face.processEvents(time::milliseconds::zero(), keepThread);
    } catch (const std::exception &e) {
        NDN_LOG_ERROR("Catch exception: "
                      << e.what() << " while processing NDN face events");
        m_error = true;
        m_pipeline->stop();
    }
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

int Consumer::validateData(const int errcode, const Interest &interest,
                           const Data &data) {
    NDN_LOG_TRACE("Validating Data for Interest: " << interest);

    if (errcode != XRDNDN_ESUCCESS) {
        NDN_LOG_ERROR("Error occured while procesing Interest: " << interest);
        return errcode;
    }

    int retValidate = XRDNDN_ESUCCESS;
    m_validator.validate(
        data,
        [&](const Data &data) {
            if (data.getContentType() == ndn::tlv::ContentType_Nack) {
                NDN_LOG_ERROR("Received application level NACK for Interest: "
                              << interest);
                retValidate = -readNonNegativeInteger(data.getContent());
            }
        },
        [&](const Data &, const security::v2::ValidationError &error) {
            NDN_LOG_ERROR("Error: " << error.getInfo()
                                    << " while validating Data for Interest: "
                                    << interest);
            retValidate = XRDNDN_EFAILURE;
        });

    return retValidate;
}

/*****************************************************************************/
/*                                  O p e n                                  */
/*****************************************************************************/
int Consumer::Open(std::string path) {
    auto openInterest =
        this->getInterest(xrdndn::SYS_CALL_OPEN_PREFIX_URI, path);

    NDN_LOG_INFO("Request open file: " << path
                                       << " with Interest: " << openInterest);

    FutureType future = m_pipeline->insert(openInterest);
    if (!future.valid()) {
        NDN_LOG_ERROR("Received invalid future for open request");
        return -ECONNABORTED;
    }

    try {
        future.wait();
    } catch (const std::exception &e) {
        NDN_LOG_ERROR("Catch exception: "
                      << e.what()
                      << " while waiting for future on open request");
        return -ECONNABORTED;
    }

    DataTypeTuple openResult = future.get();
    int retOpen = validateData(std::get<0>(openResult), std::get<1>(openResult),
                               std::get<2>(openResult));

    if (retOpen == XRDNDN_ESUCCESS) {
        retOpen = -readNonNegativeInteger(std::get<2>(openResult).getContent());
    }

    NDN_LOG_INFO("Open file: " << path << " with error code: " << retOpen);

    return retOpen;
}

/*****************************************************************************/
/*                                 C l o s e                                 */
/*****************************************************************************/
int Consumer::Close(std::string path) {
    NDN_LOG_INFO("Close file: " << path << " with error code: 0");
    m_pipeline->getStatistics(path);
    return XRDNDN_ESUCCESS;
}

/*****************************************************************************/
/*                                F s t a t                                  */
/*****************************************************************************/
int Consumer::Fstat(struct stat *buff, std::string path) {
    auto fstatInterest =
        this->getInterest(xrdndn::SYS_CALL_FSTAT_PREFIX_URI, path, 0, 16_s);

    NDN_LOG_INFO("Request fstat for file: " << path << " with Interest: "
                                            << fstatInterest);

    FutureType future = m_pipeline->insert(fstatInterest);
    if (!future.valid()) {
        NDN_LOG_ERROR("Received invalid future for fstat request");
        return -ECONNABORTED;
    }

    try {
        future.wait();
    } catch (const std::exception &e) {
        NDN_LOG_ERROR("Catch exception: "
                      << e.what()
                      << " while waiting for future on fstat request");
        return -ECONNABORTED;
    }

    DataTypeTuple fstatResult = future.get();
    int retFstat =
        validateData(std::get<0>(fstatResult), std::get<1>(fstatResult),
                     std::get<2>(fstatResult));

    if (retFstat == XRDNDN_ESUCCESS) {
        memcpy((uint8_t *)buff, std::get<2>(fstatResult).getContent().value(),
               sizeof(struct stat));
    }

    NDN_LOG_INFO("Fstat file: " << path << " with error code: " << retFstat);

    return retFstat;
}

/*****************************************************************************/
/*                                  R e a d                                  */
/*****************************************************************************/
ssize_t Consumer::Read(void *buff, off_t offset, size_t blen,
                       std::string path) {
    NDN_LOG_INFO("Reading " << blen << " bytes @" << offset
                            << " from file: " << path);

    off_t firstSegmentIdx = offset / XRDNDN_MAX_NDN_PACKET_SIZE;
    off_t lastSegmentIdx =
        ceil((offset + blen) / static_cast<double>(XRDNDN_MAX_NDN_PACKET_SIZE));

    std::vector<FutureType> futures;
    for (auto i = firstSegmentIdx; i < lastSegmentIdx; ++i) {
        auto future = m_pipeline->insert(
            getInterest(xrdndn::SYS_CALL_READ_PREFIX_URI, path, i));

        if (future.valid()) {
            futures.push_back(std::move(future));
        } else {
            NDN_LOG_ERROR("Received invalid future for read request");
            return -ECONNABORTED;
        }
    }

    std::map<uint64_t, const ndn::Block> dataStore;
    for (auto it = futures.begin(); it != futures.end(); ++it) {
        try {
            auto readResult = it->get();
            auto retRead =
                validateData(std::get<0>(readResult), std::get<1>(readResult),
                             std::get<2>(readResult));

            if (retRead != XRDNDN_ESUCCESS) {
                return retRead;
            }
            dataStore.insert(std::pair<uint64_t, const Block>(
                xrdndn::Utils::getSegmentNo(std::get<2>(readResult).getName()),
                std::get<2>(readResult).getContent()));
        } catch (const std::exception &e) {
            NDN_LOG_ERROR("Catch exception: "
                          << e.what()
                          << " while waiting for future on read request");
            return -ECONNABORTED;
        }
    }

    auto retRead = this->returnData(buff, offset, blen, std::ref(dataStore));
    NDN_LOG_TRACE("Received read Data for " << blen << " bytes @" << offset
                                            << " from file: " << path
                                            << " with ret: " << retRead);

    return retRead;
}

// todo: make this better, without map and shit
inline size_t
Consumer::returnData(void *buff, off_t offset, size_t blen,
                     std::map<uint64_t, const ndn::Block> &dataStore) {
    size_t nBytes = 0;

    auto putData = [&](const Block &content, size_t contentoff) {
        auto len = std::min(content.value_size() - contentoff, blen);
        memcpy((uint8_t *)buff + nBytes, content.value() + contentoff, len);
        nBytes += len;
        blen -= len;
    };

    auto it = dataStore.begin();
    // Store first bytes in buffer from offset
    putData(it->second, offset % XRDNDN_MAX_NDN_PACKET_SIZE);
    it = dataStore.erase(it);

    // Store rest of bytes until end
    for (; it != dataStore.end(); it = dataStore.erase(it)) {
        putData(it->second, 0);
    }

    return nBytes;
}
} // namespace xrdndnconsumer