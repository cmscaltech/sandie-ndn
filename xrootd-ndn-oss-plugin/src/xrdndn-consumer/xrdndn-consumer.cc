// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
//
// Author: Catalin Iordache <catalin.iordache@cern.ch>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <algorithm>

#include "../common/xrdndn-logger.hh"
#include "../common/xrdndn-utils.hh"
#include "xrdndn-consumer.hh"

using namespace ndn;

namespace xrdndnconsumer {
std::shared_ptr<Consumer>
Consumer::getXrdNdnConsumerInstance(const Options &opts) {
    auto consumer = std::make_shared<Consumer>(opts);

    if (!consumer || consumer->m_error) {
        NDN_LOG_FATAL("Unable to get XRootD NDN Consumer object instance");
        return nullptr;
    }

    return consumer;
}

Consumer::Consumer(const Options &opts)
    : m_options(opts), m_interestLifetime(opts.interestLifetime),
      m_validator(security::v2::getAcceptAllValidator()), m_error(false) {
    setLogLevel();
    NDN_LOG_TRACE("Alloc XRootD NDN Consumer");

    m_pipeline = std::make_shared<Pipeline>(m_face, m_options.pipelineSize);
    if (!m_pipeline) {
        m_error = true;
        NDN_LOG_ERROR("Unable to get Pipeline object instance");
        return;
    }

    processEvents(false);

    if (!m_error) {
        faceProcessEventsThread =
            boost::thread(std::bind(&Consumer::processEvents, this, true));
    }
}

Consumer::~Consumer() {
    if (m_pipeline)
        m_pipeline->stop();

    m_face.removeAllPendingInterests();
    m_face.shutdown();
    faceProcessEventsThread.join();
}

void Consumer::setLogLevel() {
    try {
        ndn::util::Logging::setLevel(CONSUMER_LOGGER_PREFIX "=" +
                                     m_options.logLevel);
    } catch (const std::invalid_argument &e) {
        std::cerr << "Catch exception: " << e.what()
                  << ". The log level will be set to implicit value NONE"
                  << std::endl;
        ndn::util::Logging::setLevel(CONSUMER_LOGGER_PREFIX "=NONE");
    }
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

const Interest Consumer::getInterest(ndn::Name prefix, uint64_t segmentNo) {
    auto name = xrdndn::Utils::getName(prefix, m_path, segmentNo);

    Interest interest(name);
    interest.setInterestLifetime(m_interestLifetime);
    interest.setMustBeFresh(true);
    interest.setDefaultCanBePrefix(false);
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
    if (path.empty()) {
        NDN_LOG_ERROR("File path is empty");
        m_error = true;
        return -ENOENT;
    }
    m_path = path;

    auto openInterest = this->getInterest(xrdndn::SYS_CALL_OPEN_PREFIX_URI);

    NDN_LOG_INFO("Request open file: " << m_path
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

    NDN_LOG_INFO("Open file: " << m_path << " with error code: " << retOpen);

    return retOpen;
}

/*****************************************************************************/
/*                                 C l o s e                                 */
/*****************************************************************************/
int Consumer::Close() {
    NDN_LOG_INFO("Close file: " << m_path << " with error code: 0");
    m_pipeline->getStatistics(m_path);
    return XRDNDN_ESUCCESS;
}

/*****************************************************************************/
/*                                F s t a t                                  */
/*****************************************************************************/
int Consumer::Fstat(struct stat *buff) {
    auto fstatInterest = this->getInterest(xrdndn::SYS_CALL_FSTAT_PREFIX_URI);

    NDN_LOG_INFO("Request fstat for file: " << m_path << " with Interest: "
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

    NDN_LOG_INFO("Fstat file: " << m_path << " with error code: " << retFstat);

    return retFstat;
}

/*****************************************************************************/
/*                                  R e a d                                  */
/*****************************************************************************/
ssize_t Consumer::Read(void *buff, off_t offset, size_t blen) {
    NDN_LOG_TRACE("Reading " << blen << " bytes @" << offset
                             << " from file: " << m_path);

    off_t firstSegmentIdx = offset / XRDNDN_MAX_PAYLOAD_SIZE;
    off_t lastSegmentIdx =
        ceil((offset + blen) / static_cast<double>(XRDNDN_MAX_PAYLOAD_SIZE));

    std::vector<FutureType> futures;
    for (auto i = firstSegmentIdx; i < lastSegmentIdx; ++i) {
        auto future = m_pipeline->insert(
            getInterest(xrdndn::SYS_CALL_READ_PREFIX_URI, i));

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
                                            << " from file: " << m_path
                                            << " with ret: " << retRead);

    return retRead;
}

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
    putData(it->second, offset % XRDNDN_MAX_PAYLOAD_SIZE);
    it = dataStore.erase(it);

    // Store rest of bytes until end
    for (; it != dataStore.end(); it = dataStore.erase(it)) {
        putData(it->second, 0);
    }

    return nBytes;
}
} // namespace xrdndnconsumer