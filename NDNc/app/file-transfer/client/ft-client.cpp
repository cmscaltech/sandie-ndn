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

#include <algorithm>
#include <queue>

#include "ft-client.hpp"
#include "logger/logger.hpp"

namespace ndnc {
namespace ft {
Client::Client(ClientOptions options,
               std::shared_ptr<PipelineInterests> pipeline)
    : m_stop{false}, m_error{false}, m_pipeline{pipeline} {
    m_options = std::make_shared<ClientOptions>(options);
    m_files = std::make_shared<std::unordered_map<std::string, uint64_t>>();
}

Client::~Client() {
    this->stop();
}

bool Client::canContinue() {
    return !m_stop && !m_error && !m_pipeline->isClosed();
}

void Client::stop() {
    m_stop = true;
}

std::shared_ptr<ndn::Data>
Client::syncRequestDataFor(std::shared_ptr<ndn::Interest> &&interest,
                           uint64_t id) {
    // Insert the Interest packet in the pipeline
    if (!m_pipeline->pushInterest(id, std::move(interest))) {
        LOG_FATAL("unable to push Interest packet to pipeline");
        m_error = true;
        return nullptr;
    }

    std::shared_ptr<ndn::Data> pkt(nullptr);
    // Wait for the response from the pipeline
    while (this->canContinue(), !m_pipeline->popData(id, pkt)) {}

    return pkt;
}

std::vector<std::shared_ptr<ndn::Data>> Client::syncRequestDataFor(
    std::vector<std::shared_ptr<ndn::Interest>> &&interests, uint64_t id) {
    auto npkts = interests.size();

    if (!m_pipeline->pushInterestBulk(id, std::move(interests))) {
        LOG_FATAL("unable to push Interest packets to pipeline");
        m_error = true;
        return {};
    }

    std::vector<std::shared_ptr<ndn::Data>> pkts(npkts);

    for (; npkts > 0; --npkts) {
        std::shared_ptr<ndn::Data> pkt(nullptr);

        while (this->canContinue() && !m_pipeline->popData(id, pkt)) {}

        if (pkt == nullptr) {
            return {};
        }

        if (!pkt->getName().at(-1).isSegment()) {
            LOG_FATAL("last name component of Data packet is not a segment");
            m_error = true;
            return {};
        }

        try {
            pkts[pkt->getName().at(-1).toSegment()] = pkt;
        } catch (ndn::Name::Error &error) {
            LOG_FATAL("unable to get the segment from the Data Name");
            m_error = true;
            return {};
        }
    }

    return pkts;
}

bool Client::asyncRequestDataFor(std::shared_ptr<ndn::Interest> &&interest,
                                 uint64_t id) {
    interest->setInterestLifetime(m_options->lifetime);

    if (!m_pipeline->pushInterest(id, std::move(interest))) {
        LOG_FATAL("unable to push Interest packet to pipeline");
        m_error = true;
        return false;
    }

    return true;
}

bool Client::asyncRequestDataFor(
    std::vector<std::shared_ptr<ndn::Interest>> &&interests, uint64_t id) {

    if (!m_pipeline->pushInterestBulk(id, std::move(interests))) {
        LOG_FATAL("unable to push Interest packets to pipeline");
        m_error = true;
        return false;
    }

    return true;
}

void Client::openFile(std::shared_ptr<FileMetadata> metadata) {
    auto id = m_pipeline->registerConsumer();
    m_files->emplace(metadata->getVersionedName().toUri(), id);
}

void Client::closeFile(std::shared_ptr<FileMetadata> metadata) {
    auto it = m_files->find(metadata->getVersionedName().toUri());

    if (it == m_files->end()) {
        LOG_DEBUG("trying to close an unopened file");
        return;
    }

    m_pipeline->unregisterConsumer(it->second);
    m_files->erase(it);
}

void Client::listFile(std::string path,
                      std::shared_ptr<FileMetadata> &metadata) {
    metadata = nullptr;

    auto interest = std::make_shared<ndn::Interest>(
        rdrDiscoveryNameFileRetrieval(path, m_options->namePrefix));

    interest->setCanBePrefix(true);
    interest->setMustBeFresh(true);
    interest->setInterestLifetime(m_options->lifetime);

    auto data = syncRequestDataFor(std::move(interest));

    if (data == nullptr || !data->hasContent()) {
        LOG_ERROR("invalid data");
        m_error = true;
        return;
    }

    if (data->getContentType() == ndn::tlv::ContentType_Nack) {
        LOG_ERROR("unable to list file '%s'", path.c_str());
        return;
    }

    metadata = std::make_shared<FileMetadata>(data->getContent());
}

void Client::listDir(std::string root,
                     std::vector<std::shared_ptr<FileMetadata>> &all) {
    std::shared_ptr<FileMetadata> metadata(nullptr);
    all.clear();

    {
        // Get list dir Metadata
        auto interest = std::make_shared<ndn::Interest>(
            rdrDiscoveryNameDirListing(root, m_options->namePrefix));

        interest->setCanBePrefix(true);
        interest->setMustBeFresh(true);
        interest->setInterestLifetime(m_options->lifetime);

        auto data = syncRequestDataFor(std::move(interest));

        if (data == nullptr || !data->hasContent()) {
            LOG_ERROR("invalid data");
            m_error = true;
            return;
        }

        if (data->getContentType() == ndn::tlv::ContentType_Nack) {
            LOG_ERROR("unable to list dir: '%s'", root.c_str());
            return;
        }

        metadata = std::make_shared<FileMetadata>(data->getContent());
    }

    if (metadata == nullptr) {
        LOG_ERROR("unable to list dir: '%s'", root.c_str());
        return;
    }

    if (!metadata->isDir()) {
        LOG_ERROR("request to list dir on a file path: '%s'", root.c_str());
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
                LOG_ERROR("invalid data");
                m_error = true;
                return;
            }

            if (data->getContentType() == ndn::tlv::ContentType_Nack) {
                LOG_FATAL("unable to get list dir content '%s'", root.c_str());
                m_error = true;
                return;
            } else {
                if (!data->getFinalBlock()->isSegment()) {
                    LOG_ERROR(
                        "dir list data content FinalBlockId is not a segment");
                    m_error = true;
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
        auto prefix = root.back() == '/' ? root : root + "/";

        content.push_back(prefix + suffix);
        i = j + 1;
    }

    free(byteContent);

    // Request Metadata for all files in the directory
    for (auto file : content) {
        metadata = nullptr;
        listFile(file, metadata);

        if (metadata == nullptr) {
            LOG_ERROR("null metadata");
            continue;
        } else {
            all.push_back(metadata);
        }
    }
}

void Client::listDirRecursive(std::string root,
                              std::vector<std::shared_ptr<FileMetadata>> &all) {
    std::queue<std::string> paths;
    paths.push(root);

    while (!paths.empty()) {
        auto path = paths.front();
        paths.pop();

        std::vector<std::shared_ptr<FileMetadata>> metadata{};
        listDir(path, metadata);

        if (metadata.empty()) {
            continue;
        }

        for (auto md : metadata) {
            all.push_back(md);

            if (md->isDir()) {
                paths.push(ndnc::rdrDirUri(md->getVersionedName()));
            }
        }
    }

    std::sort(all.begin(), all.end(),
              [](std::shared_ptr<FileMetadata> lhs,
                 std::shared_ptr<FileMetadata> rhs) {
                  return ndnc::rdrDirUri(lhs->getVersionedName()) <
                         ndnc::rdrDirUri(rhs->getVersionedName());
              });
}

void Client::requestFileContent(std::shared_ptr<FileMetadata> metadata) {
    uint64_t npkts = 64;
    uint64_t id = m_files->at(metadata->getVersionedName().toUri());

    for (uint64_t segmentNo = 0;
         segmentNo <= metadata->getFinalBlockID() && this->canContinue();) {

        // if (m_pipeline->getQueuedInterestsCount() > 65536) {
        //     // backoff; plenty of work to be done by the pipeline
        //     continue;
        // }

        std::vector<std::shared_ptr<ndn::Interest>> pkts;
        pkts.reserve(npkts);

        for (uint64_t nextSegment = segmentNo, n = 0;
             nextSegment <= metadata->getFinalBlockID() && n < npkts;
             ++nextSegment, ++n) {

            auto interest = std::make_shared<ndn::Interest>(
                metadata->getVersionedName().deepCopy().appendSegment(
                    nextSegment));

            interest->setInterestLifetime(m_options->lifetime);
            pkts.emplace_back(std::move(interest));
        }

        if (!asyncRequestDataFor(std::move(pkts), id)) {
            m_error = true;
            return;
        }

        segmentNo += npkts;

        // std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void Client::receiveFileContent(NotifyProgressStatus onProgress,
                                std::shared_ptr<FileMetadata> metadata) {
    uint64_t bytesCount = 0;
    uint64_t segmentsCount = 0;
    uint64_t id = m_files->at(metadata->getVersionedName().toUri());

    while (this->canContinue() &&
           segmentsCount <= metadata->getFinalBlockID()) {

        size_t npkts = 16;
        std::vector<std::shared_ptr<ndn::Data>> pkts(npkts);
        npkts = m_pipeline->popDataBulk(id, pkts);

        if (npkts == 0) {
            continue;
        }

        for (size_t i = 0; i < npkts; ++i) {
            if (pkts[i] == nullptr) {
                LOG_FATAL("pipeline error on receive file content");
                m_error = true;
                return;
            }

            bytesCount += pkts[i]->getContent().value_size();
        }

        segmentsCount += npkts;

        if (bytesCount > 5242880) {
            onProgress(bytesCount);
            bytesCount = 0;
        }
    }

    onProgress(bytesCount);
}
}; // namespace ft
}; // namespace ndnc
