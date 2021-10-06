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

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>

#include "ft-client.hpp"

namespace ndnc {
namespace benchmark {
namespace ft {
Runner::Runner(Face &face, ClientOptions options)
    : m_options(options), m_counters(), m_npackets(64), m_shouldStop(false),
      m_hasError(false) {
    switch (m_options.pipelineType) {
    case fixed:
    default:
        m_pipeline = new PipelineFixed(face, m_options.pipelineSize);
    }
}

Runner::~Runner() {
    m_shouldStop = true;
    wait();

    if (m_pipeline != nullptr) {
        delete m_pipeline;
    }
}

void Runner::run(NotifyProgressStatus onProgress) {
    for (auto i = 0; i < m_options.nthreads; ++i) {
        m_workers.push_back(
            std::thread(&Runner::getFileContent, this, i, onProgress));
    }
}

void Runner::wait() {
    if (m_workers.empty()) {
        return;
    }
    for (auto i = 0; i < m_options.nthreads; ++i) {
        if (m_workers[i].joinable()) {
            m_workers[i].join();
        }
    }
}

void Runner::stop() {
    m_shouldStop = true;
    if (m_pipeline != nullptr) {
        m_pipeline->stop();
    }
}

bool Runner::isValid() {
    return !m_shouldStop && !m_hasError && m_pipeline->isValid();
}

uint64_t Runner::getFileMetadata() {
    auto interest =
        std::make_shared<ndn::Interest>(getNameForMetadata(m_options.file));
    interest->setCanBePrefix(true);
    interest->setMustBeFresh(true);

    // Express Interest
    RxQueue rxQueue;
    if (expressInterests(std::move(interest), &rxQueue) == 0) {
        m_hasError = true;
        return 0;
    }

    // Wait for Data
    PendingInterestResult result;
    while (!rxQueue.wait_dequeue_timed(result, 1000)) {
        if (isValid()) {
            continue;
        } else {
            return 0;
        }
    }

    if (!isValid()) {
        return 0;
    }

    if (result.hasError()) {
        if (result.getErrorCode() == NETWORK) {
            std::cout << "ERROR: network is unrecheable\n";
        }
        m_hasError = true;
        return 0;
    }

    m_counters.nData.fetch_add(1, std::memory_order_release);

    if (!result.getData()->hasContent() ||
        result.getData()->getContentType() == ndn::tlv::ContentType_Nack) {
        std::cout << "FATAL: Could not open file: " << m_options.file << "\n";
        m_hasError = true;
        return 0;
    }

    m_metadata = FileMetadata(result.getData()->getContent());

    std::cout << "file " << m_options.file << " of size "
              << m_metadata.getFileSize() << " bytes ("
              << m_metadata.getSegmentSize() << "/"
              << m_metadata.getLastSegment() << ") and latest version="
              << m_metadata.getVersionedName().get(-1).toVersion() << "\n";

    return m_metadata.getFileSize();
}

void Runner::getFileContent(int tid, NotifyProgressStatus onProgress) {
    RxQueue rxQueue;

    for (uint64_t segmentNo = tid * m_npackets;
         segmentNo <= m_metadata.getLastSegment() && isValid();
         segmentNo += m_options.nthreads * m_npackets) {

        uint8_t nTx = 0;
        for (uint64_t nextSegment = segmentNo;
             nextSegment <= m_metadata.getLastSegment() && nTx < m_npackets;
             ++nextSegment, ++nTx) {

            auto interest = std::make_shared<ndn::Interest>(
                m_metadata.getVersionedName().deepCopy().appendSegment(
                    nextSegment));

            if (expressInterests(std::move(interest), &rxQueue) == 0) {
                m_hasError = true;
                return;
            }
        }

        uint64_t nBytes = 0;
        for (; nTx > 0 && isValid();) {
            PendingInterestResult result;
            if (!rxQueue.wait_dequeue_timed(result, 1000)) {
                if (isValid()) {
                    continue;
                } else {
                    return;
                }
            }
            --nTx;
            if (result.hasError()) {
                if (result.getErrorCode() == NETWORK) {
                    std::cout << "ERROR: network is unrecheable\n";
                }
                m_hasError = true;
                return;
            }

            m_counters.nData.fetch_add(1, std::memory_order_release);
            nBytes += result.getData()->getContent().value_size();
        }

        onProgress(nBytes);
    }
}

int Runner::expressInterests(std::shared_ptr<ndn::Interest> &&interest,
                             RxQueue *rxQueue) {
    interest->setInterestLifetime(m_options.lifetime);

    if (!m_pipeline->enqueueInterestPacket(std::move(interest), rxQueue)) {
        std::cout << "FATAL: unable to enqueue Interest \n";
        m_hasError = true;
        return 0;
    }

    m_counters.nInterest.fetch_add(1, std::memory_order_release);
    return 1;
}

Runner::Counters &Runner::readCounters() {
    return this->m_counters;
}
}; // namespace ft
}; // namespace benchmark
}; // namespace ndnc
