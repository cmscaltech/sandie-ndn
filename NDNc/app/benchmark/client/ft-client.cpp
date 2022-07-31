/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2021 California Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "ft-client.hpp"
#include "logger/logger.hpp"

namespace ndnc {
namespace benchmark {
namespace ft {
Runner::Runner(face::Face &face, ClientOptions options)
    : m_stop{false}, m_error{false} {
    m_options = std::make_shared<ClientOptions>(options);

    switch (m_options->pipelineType) {
    case aimd:
        m_pipeline = std::make_shared<PipelineInterestsAimd>(
            face, m_options->pipelineSize);
        break;
    case fixed:
    default:
        m_pipeline = std::make_shared<PipelineInterestsFixed>(
            face, m_options->pipelineSize);
    }

    m_pipeline->run();
}

Runner::~Runner() {
    this->stop();
}

bool Runner::canContinue() {
    return !m_stop && !m_error && m_pipeline->isValid();
}

void Runner::stop() {
    m_stop = true;

    if (m_pipeline != nullptr && m_pipeline->isValid()) {
        m_pipeline->stop();
    }
}

std::shared_ptr<PipelineInterests::Counters> Runner::getCounters() {
    return m_pipeline->getCounters();
}

std::shared_ptr<ndn::Data>
Runner::syncRequestDataFor(std::shared_ptr<ndn::Interest> &&interest) {
    // Insert the Interest packet in the pipeline
    if (!m_pipeline->enqueueInterest(std::move(interest))) {
        LOG_FATAL("unable to insert Interest packet in the pipeline");
        return nullptr;
    }

    // Wait for the response from the pipeline
    std::shared_ptr<ndn::Data> packet(nullptr);
    while (this->canContinue() && !m_pipeline->dequeueData(packet)) {}

    return packet;
}

std::vector<std::shared_ptr<ndn::Data>> Runner::syncRequestDataFor(
    std::vector<std::shared_ptr<ndn::Interest>> &&interests) {
    auto npackets = interests.size();

    if (!m_pipeline->enqueueInterests(std::move(interests))) {
        LOG_FATAL("unable to insert Interest packets in the pipeline");
        return {};
    }

    std::vector<std::shared_ptr<ndn::Data>> packets(npackets);

    for (; npackets > 0; --npackets) {
        std::shared_ptr<ndn::Data> packet(nullptr);
        while (this->canContinue() && !m_pipeline->dequeueData(packet)) {}

        if (!this->canContinue() || packet == nullptr) {
            return {};
        }

        if (!packet->getName().at(-1).isSegment()) {
            LOG_FATAL("last name component of Data packet is not a segment");
            m_error = true;
            return {};
        }

        try {
            packets[packet->getName().at(-1).toSegment()] = packet;
        } catch (ndn::Name::Error &error) {
            LOG_FATAL("unable to get the segment from the Data Name");
            return {};
        }
    }

    return packets;
}

void Runner::listFile(std::string file,
                      std::shared_ptr<FileMetadata> &metadata) {
    metadata = nullptr;

    auto interest = std::make_shared<ndn::Interest>(
        rdrDiscoveryNameFileRetrieval(file, m_options->namePrefix));

    interest->setCanBePrefix(true);
    interest->setMustBeFresh(true);
    interest->setInterestLifetime(m_options->lifetime);

    auto data = syncRequestDataFor(std::move(interest));

    if (data == nullptr || !data->hasContent()) {
        return;
    }

    if (data->getContentType() == ndn::tlv::ContentType_Nack) {
        LOG_ERROR("unable to list file '%s'", file.c_str());
        return;
    }

    metadata = std::make_shared<FileMetadata>(data->getContent());

    // TODO: Remove
    // if (metadata->isFile()) {
    //     LOG_INFO("file: '%s' size=%li (%lix%li) versioned name: %s",
    //              file.c_str(), metadata->getFileSize(),
    //              metadata->getSegmentSize(), metadata->getFinalBlockID(),
    //              metadata->getVersionedName().toUri().c_str());
    // } else {
    //     LOG_INFO("dir: '%s'", file.c_str());
    // }
}

void Runner::listDir(std::string dir, // TODO -> path
                     std::vector<std::shared_ptr<FileMetadata>> &all) {
    std::shared_ptr<FileMetadata> metadata(nullptr);

    all.clear();

    {
        // Get list dir Metadata
        auto interest = std::make_shared<ndn::Interest>(
            rdrDiscoveryNameDirListing(dir, m_options->namePrefix));

        interest->setCanBePrefix(true);
        interest->setMustBeFresh(true);
        interest->setInterestLifetime(m_options->lifetime);

        auto data = syncRequestDataFor(std::move(interest));

        if (data == nullptr || !data->hasContent()) {
            return;
        }

        if (data->getContentType() == ndn::tlv::ContentType_Nack) {
            LOG_ERROR("unable to list dir: '%s'", dir.c_str());
            return;
        }

        metadata = std::make_shared<FileMetadata>(data->getContent());
    }

    if (metadata == nullptr) {
        LOG_ERROR("unable to list dir: '%s'", dir.c_str());
        return;
    }

    if (!metadata->isDir()) {
        LOG_ERROR("request to list dir on a file path: '%s'", dir.c_str());
        return;
    }

    uint8_t *byteContent =
        (uint8_t *)malloc(8800 * (metadata->getFinalBlockID() + 1));

    if (NULL == byteContent) {
        LOG_FATAL("unable to allocate memory for retrieving dir list content");
        m_error = true;
        return;
    }

    uint64_t byteContentOffset = 0;
    uint64_t contentFinalBlockId = 0;
    {
        // Get all list dir content
        for (uint64_t i = 0; i <= contentFinalBlockId; ++i) {
            auto interest = std::make_shared<ndn::Interest>(
                metadata->getVersionedName().appendSegment(i));

            interest->setInterestLifetime(m_options->lifetime);

            auto data = syncRequestDataFor(std::move(interest));

            if (data == nullptr || !data->hasContent()) {
                return;
            }

            if (data->getContentType() == ndn::tlv::ContentType_Nack) {
                LOG_FATAL("unable to get list dir content '%s'", dir.c_str());
                return;
            } else {
                if (!data->getFinalBlock()->isSegment()) {
                    LOG_ERROR(
                        "dir list data content FinalBlockId is not a segment");
                    return;
                } else {
                    contentFinalBlockId = data->getFinalBlock()->toSegment();
                }
            }

            memcpy(byteContent + byteContentOffset, data->getContent().value(),
                   data->getContent().value_size());
            byteContentOffset += data->getContent().value_size();
        }
    }

    // Parse dir list byteContent to a list of paths
    std::vector<std::string> content = {};
    for (uint64_t i = 0, j = 0;
         j <= byteContentOffset && i <= byteContentOffset; ++j) {
        if (byteContent[j] != '\0') {
            continue;
        }

        if (byteContent[i] == '\0') {
            break;
        }

        auto suffix = std::string((const char *)(byteContent + i), j - i);
        auto prefix = dir.back() == '/' ? dir : dir + "/";

        content.push_back(prefix + suffix);
        i = j + 1;
    }

    free(byteContent);

    // Request Metadata for all files in the directory
    for (auto file : content) {
        metadata = nullptr;
        listFile(file, metadata);

        if (metadata == nullptr) {
            continue;
        } else {
            all.push_back(metadata);
        }
    }
}

void Runner::listDirRecursive(std::string dir,
                              std::vector<std::shared_ptr<FileMetadata>> &all) {
    {
        std::shared_ptr<FileMetadata> metadata(nullptr);
        listFile(dir, metadata);

        if (metadata == nullptr) {
            return;
        }

        if (metadata->isFile()) {
            all.push_back(metadata);
            return;
        }
    }

    {
        std::vector<std::shared_ptr<FileMetadata>> metadata{};
        listDir(dir, metadata);

        for (auto md : metadata) {
            if (md->isFile()) {
                all.push_back(md);
            } else {
                listDirRecursive(ndnc::rdrDir(md->getVersionedName()), all);
            }
        }
    }
}

void Runner::requestFileContent(int wid, int wcount, uint64_t finalBlockID,
                                ndn::Name name) {
    uint64_t npackets = 64;

    for (uint64_t segmentNo = wid * npackets;
         segmentNo <= finalBlockID && canContinue();) {

        if (m_pipeline->getPendingRequestsCount() > 65536) {
            // backoff; plenty of work to be done by the pipeline
            continue;
        }

        std::vector<std::shared_ptr<ndn::Interest>> interests;
        interests.reserve(npackets);

        for (uint64_t nextSegment = segmentNo, n = 0;
             nextSegment <= finalBlockID && n < npackets; ++nextSegment, ++n) {

            auto interest = std::make_shared<ndn::Interest>(
                name.deepCopy().appendSegment(nextSegment));

            interest->setInterestLifetime(m_options->lifetime);
            interests.emplace_back(std::move(interest));
        }

        if (!requestData(std::move(interests))) {
            m_error = true;
            return;
        }

        segmentNo += wcount * npackets;
    }
}

bool Runner::requestData(std::shared_ptr<ndn::Interest> &&interest) {
    interest->setInterestLifetime(m_options->lifetime);

    if (!m_pipeline->enqueueInterest(std::move(interest))) {
        LOG_FATAL("unable to enqueue Interest packet");
        return false;
    }

    return true;
}

bool Runner::requestData(
    std::vector<std::shared_ptr<ndn::Interest>> &&interests) {

    // TODO: async_request

    if (!m_pipeline->enqueueInterests(std::move(interests))) {
        LOG_FATAL("unable to enqueue Interest packets");
        return false;
    }

    return true;
}

void Runner::receiveFileContent(NotifyProgressStatus onProgress,
                                std::atomic<uint64_t> &segmentsCount,
                                uint64_t finalBlockID) {
    uint64_t bytesCount = 0;

    while (canContinue() && segmentsCount <= finalBlockID) {
        std::shared_ptr<ndn::Data> data;
        if (!m_pipeline->dequeueData(data)) {
            continue;
        }

        if (data == nullptr) {
            LOG_FATAL("pipeline error on receive file content");
            return;
        }

        bytesCount += data->getContent().value_size();
        segmentsCount += 1;

        if (bytesCount > 5242880) {
            onProgress(bytesCount);
            bytesCount = 0;
        }
    }

    onProgress(bytesCount);
}
}; // namespace ft
}; // namespace benchmark
}; // namespace ndnc
