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
    : m_stop{false}, m_error{false}, m_metadata{nullptr} {
    m_options = std::make_shared<ClientOptions>(options);
    m_counters = std::make_shared<Counters>();

    switch (m_options->pipelineType) {
    case fixed:
    default:
        m_pipeline =
            std::make_shared<PipelineFixed>(face, m_options->pipelineSize);
    }

    m_pipeline->start();
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
    RxQueue rxQueue;
    if (request(std::move(interest), &rxQueue) == 0) {
        m_error = true;
        return;
    }

    // Wait for Data
    PendingInterestResult response;
    while (!rxQueue.wait_dequeue_timed(response, 500) && canContinue()) {
    }

    if (!canContinue())
        return;

    m_counters->addData();

    if (response.hasError()) {
        if (response.getErrorCode() == NETWORK) {
            std::cout << "ERROR: network is unrecheable\n";
        }
        m_error = true;
        return;
    }

    if (!response.getData()->hasContent() ||
        response.getData()->getContentType() == ndn::tlv::ContentType_Nack) {

        std::cout << "FATAL: Could not open file: " << m_options->file << "\n";
        m_error = true;
        return;
    }

    m_metadata =
        std::make_shared<FileMetadata>(response.getData()->getContent());
    *size = m_metadata->getFileSize();

    std::cout << "file " << m_options->file << " of size "
              << m_metadata->getFileSize() << " bytes ("
              << m_metadata->getSegmentSize() << "/"
              << m_metadata->getLastSegment() << ") and latest version="
              << m_metadata->getVersionedName().get(-1).toVersion() << "\n";
}

void Runner::getFileContent(int wid, NotifyProgressStatus onProgress) {
    if (m_metadata == nullptr) {
        std::cout
            << "ERROR: consumer doesn't have a reference to valid metadata\n";
        return;
    }

    RxQueue rxQueue;
    size_t npackets = 64;

    for (uint64_t segmentNo = wid * npackets;
         segmentNo <= m_metadata->getLastSegment() && canContinue();
         segmentNo += m_options->nthreads * npackets) {

        size_t nTx = 0;
        for (uint64_t nextSegment = segmentNo;
             nextSegment <= m_metadata->getLastSegment() && nTx < npackets;
             ++nextSegment, ++nTx) {

            auto interest = std::make_shared<ndn::Interest>(
                m_metadata->getVersionedName().deepCopy().appendSegment(
                    nextSegment));

            if (request(std::move(interest), &rxQueue) == 0) {
                m_error = true;
                return;
            }
        }

        uint64_t nBytes = 0;

        for (; nTx > 0 && canContinue();) {
            PendingInterestResult response;
            while (!rxQueue.wait_dequeue_timed(response, 500) &&
                   canContinue()) {
            }

            if (!canContinue())
                return;

            m_counters->addData();
            --nTx;

            if (response.hasError()) {
                if (response.getErrorCode() == NETWORK) {
                    std::cout << "ERROR: network is unrecheable\n";
                }
                m_error = true;
                return;
            } else {
                nBytes += response.getData()->getContent().value_size();
            }
        }

        onProgress(nBytes);
    }
}

size_t Runner::request(std::shared_ptr<ndn::Interest> &&interest,
                       RxQueue *rxQueue) {
    interest->setInterestLifetime(m_options->lifetime);

    if (canContinue() &&
        !m_pipeline->enqueueInterestPacket(std::move(interest), rxQueue)) {
        std::cout << "FATAL: unable to enqueue Interest \n";
        m_error = true;
        return 0;
    }

    m_counters->addInterest();
    return 1;
}

size_t Runner::request(std::vector<std::shared_ptr<ndn::Interest>> &&,
                       RxQueue *) {
    // TODO
    return 0;
}
}; // namespace ft
}; // namespace benchmark
}; // namespace ndnc
