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
#include "influxdb_upload.hpp"
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

std::shared_ptr<PipelineInterests::Counters> Runner::readCounters() {
    return m_pipeline->counters();
}

bool Runner::getFileMetadata(FileMetadata &metadata) {
    // Compose Interest packet
    auto interest = std::make_shared<ndn::Interest>(
        rdrDiscoveryInterestNameFromFilePath(m_options->file));

    interest->setCanBePrefix(true);
    interest->setMustBeFresh(true);

    // Express Interest
    if (!requestData(std::move(interest))) {
        return false;
    }

    // Wait for Data
    std::shared_ptr<ndn::Data> data;
    while (canContinue() && !m_pipeline->dequeueData(data)) {}

    if (!canContinue()) {
        return false;
    }

    if (data == nullptr) {
        LOG_ERROR(
            "error in pipeline"); // TODO: This can also mean RDR Nack error
        return false;
    }

    if (!data->hasContent() ||
        data->getContentType() == ndn::tlv::ContentType_Nack) {
        LOG_FATAL("unable to open file '%s'", m_options->file.c_str());
        return false;
    }

    metadata = FileMetadata(data->getContent());

    if (metadata.isFile()) {
        LOG_INFO("file: '%s' size=%li (%lix%li) versioned name: %s",
                 m_options->file.c_str(), metadata.getFileSize(),
                 metadata.getSegmentSize(), metadata.getFinalBlockId(),
                 metadata.getVersionedName().toUri().c_str());
    }

    return true;
}

void Runner::requestFileContent(int wid, FileMetadata metadata) {
    uint64_t npackets = 64;

    for (uint64_t segmentNo = wid * npackets;
         segmentNo <= metadata.getFinalBlockId() && canContinue();) {

        if (m_pipeline->getPendingRequestsCount() > 65536) {
            // backoff; plenty of work to be done by the pipeline
            continue;
        }

        std::vector<std::shared_ptr<ndn::Interest>> interests;
        interests.reserve(npackets);

        for (uint64_t nextSegment = segmentNo, n = 0;
             nextSegment <= metadata.getFinalBlockId() && n < npackets;
             ++nextSegment, ++n) {

            auto interest = std::make_shared<ndn::Interest>(
                metadata.getVersionedName().deepCopy().appendSegment(
                    nextSegment));

            interest->setInterestLifetime(m_options->lifetime);
            interests.emplace_back(std::move(interest));
        }

        if (!requestData(std::move(interests))) {
            m_error = true;
            return;
        }

        segmentNo += (m_options->nthreads * 0.5) * npackets;
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
    if (!m_pipeline->enqueueInterests(std::move(interests))) {
        LOG_FATAL("unable to enqueue Interest packets");
        return false;
    }

    return true;
}

void Runner::receiveFileContent(NotifyProgressStatus onProgress,
                                std::atomic<uint64_t> &bytesCount,
                                std::atomic<uint64_t> &segmentsCount,
                                uint64_t finalBlockId) {
    while (canContinue() && segmentsCount <= finalBlockId) {
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

        if (bytesCount % 5242880 == 0) {
            onProgress();
        }
    }

    onProgress();
}
}; // namespace ft
}; // namespace benchmark
}; // namespace ndnc
