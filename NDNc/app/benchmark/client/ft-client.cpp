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

#include <iostream>

#include "ft-client.hpp"

namespace ndnc {
namespace benchmark {
namespace ft {
Runner::Runner(Face &face, ClientOptions options)
    : m_stop{false}, m_error{false}, m_nReceived{0}, m_metadata{nullptr} {
    m_options = std::make_shared<ClientOptions>(options);
    m_counters = std::make_shared<Counters>();

    switch (m_options->pipelineType) {
    case fixed:
    default:
        m_pipeline = std::make_shared<PipelineInterestsFixed>(
            face, m_options->pipelineSize);
    }

    m_pipeline->begin();
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
        m_pipeline->end();
    }
}

std::shared_ptr<Runner::Counters> Runner::readCounters() {
    return m_counters;
}

void Runner::getFileInfo(uint64_t *size) {
    // Compose Interest packet
    auto interest =
        std::make_shared<ndn::Interest>(getNameForMetadata(m_options->file));
    interest->setInterestLifetime(m_options->lifetime);
    interest->setCanBePrefix(true);
    interest->setMustBeFresh(true);

    // Express Interest
    if (requestData(std::move(interest)) == 0) {
        m_error = true;
        return;
    }

    // Wait for Data
    auto data = onResponseData();

    if (data == nullptr || !data->hasContent() ||
        data->getContentType() == ndn::tlv::ContentType_Nack) {

        std::cout << "FATAL: could not open file: " << m_options->file << "\n";
        m_error = true;
        return;
    }

    m_metadata = std::make_shared<FileMetadata>(data->getContent());
    *size = m_metadata->getFileSize();

    std::cout << "file " << m_options->file << " of size "
              << m_metadata->getFileSize() << " bytes ("
              << m_metadata->getSegmentSize() << "/"
              << m_metadata->getLastSegment() << ") and latest version="
              << m_metadata->getVersionedName().get(-1).toVersion() << "\n";
}

void Runner::requestFileContent(int wid) {
    uint64_t npackets = 64;

    for (uint64_t segmentNo = wid * npackets;
         segmentNo <= m_metadata->getLastSegment() && canContinue();) {

        if (m_pipeline->size() > 2048) {
            std::cout << "INFO: requestData thread backoff. pit size="
                      << m_pipeline->size() << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            continue;
        }

        size_t nTx = 0;
        std::vector<std::shared_ptr<ndn::Interest>> interests;
        interests.reserve(npackets);

        for (uint64_t nextSegment = segmentNo;
             nextSegment <= m_metadata->getLastSegment() && nTx < npackets;
             ++nextSegment, ++nTx) {

            auto interest = std::make_shared<ndn::Interest>(
                m_metadata->getVersionedName().deepCopy().appendSegment(
                    nextSegment));

            interest->setInterestLifetime(m_options->lifetime);
            interests.emplace_back(std::move(interest));
        }

        if (requestData(std::move(interests), nTx) != nTx) {
            m_error = true;
            return;
        }

        segmentNo += (m_options->nthreads / 2) * npackets;
    }
}

void Runner::receiveFileContent(NotifyProgressStatus onProgress) {
    uint64_t nBytes = 0;

    for (; m_nReceived < m_metadata->getLastSegment() && canContinue();
         ++m_nReceived) {

        auto data = onResponseData();

        if (data == nullptr) {
            m_stop = true;
            return;
        }

        nBytes += data->getContent().value_size();

        if (nBytes > 2097152) {
            onProgress(nBytes);
            nBytes = 0;
        }
    }

    onProgress(nBytes);
}

size_t Runner::requestData(std::shared_ptr<ndn::Interest> &&interest) {
    interest->setInterestLifetime(m_options->lifetime);

    if (canContinue() && !m_pipeline->enqueueInterest(std::move(interest))) {
        std::cout << "FATAL: unable to enqueue Interest \n";
        m_error = true;
        return 0;
    }

    ++m_counters->nInterest;
    return 1;
}

size_t
Runner::requestData(std::vector<std::shared_ptr<ndn::Interest>> &&interests,
                    size_t n) {
    if (canContinue() && !m_pipeline->enqueueInterests(std::move(interests))) {
        std::cout << "FATAL: unable to enqueue Interests \n";
        m_error = true;
        return 0;
    }

    m_counters->nInterest += n;
    return n;
}

std::shared_ptr<ndn::Data> Runner::onResponseData() {
    while (canContinue()) {
        std::shared_ptr<ndn::Data> data;
        if (!m_pipeline->dequeueData(data)) {
            // wait for data. continue until get a response
            continue;
        }

        if (data != nullptr) {
            ++m_counters->nData;
            return data;
        }
    }

    std::cout << "ERROR: pipeline encountered an error\n";
    return nullptr;
}
}; // namespace ft
}; // namespace benchmark
}; // namespace ndnc
