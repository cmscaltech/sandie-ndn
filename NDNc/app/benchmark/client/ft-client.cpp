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
#include <iostream>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>

#include "ft-client.hpp"

namespace ndnc {
namespace benchmark {
namespace ft {
Runner::Runner(Face &face, ClientOptions options)
    : m_options(options), m_counters(), m_chunk(64), m_stop(false) {
    m_pipeline = new Pipeline(face);
}

Runner::~Runner() {
    m_stop = true;
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
    m_stop = true;
    m_pipeline->stop();
}

uint64_t Runner::getFileMetadata() {
    auto interest =
        std::make_shared<ndn::Interest>(getNameForMetadata(m_options.file));
    interest->setCanBePrefix(true);
    interest->setMustBeFresh(true);

    // Express Interest
    RxQueue rxQueue;
    if (expressInterests(interest, &rxQueue) == 0) {
        return 0;
    }

    if (m_stop) {
        return 0;
    }

    // Wait for Data
    ndn::Data data;
    if (!rxQueue.wait_dequeue_timed(data, m_options.lifetime.count() * 1000)) {
        std::cout << "Request timeout for META Interest\n";
        m_counters.nTimeout.fetch_add(1, std::memory_order_release);
        return 0;
    } else {
        m_counters.nData.fetch_add(1, std::memory_order_release);
    }

    if (!data.hasContent() ||
        data.getContentType() == ndn::tlv::ContentType_Nack) {
        std::cout << "FATAL: Could not open file: " << m_options.file << "\n";
        return 0;
    }

    m_fileMetadata = FileMetadata(data.getContent());

    std::cout << "DEBUG: " << m_options.file << "metadata:"
              << "\nversioned name: " << m_fileMetadata.getVersionedName()
              << "\nsegment size: " << m_fileMetadata.getSegmentSize()
              << "\nlast segment: " << m_fileMetadata.getLastSegment()
              << "\nfile size: " << m_fileMetadata.getFileSize()
              << "\nfile mode: " << m_fileMetadata.getMode()
              << "\natime: " << m_fileMetadata.getAtimeAsInt()
              << "\nbtime: " << m_fileMetadata.getBtimeAsInt()
              << "\nctime: " << m_fileMetadata.getCtimeAsInt()
              << "\namtime: " << m_fileMetadata.getMtimeAsInt() << "\n";

    return m_fileMetadata.getFileSize();
}

void Runner::getFileContent(int tid, NotifyProgressStatus onProgress) {
    RxQueue rxQueue;
    uint64_t segmentNo = tid;

    while (segmentNo < m_fileMetadata.getLastSegment() && !m_stop) {
        uint8_t nTx = 0;

        for (auto i = 0; i < m_chunk &&
                         segmentNo < m_fileMetadata.getLastSegment() && !m_stop;
             ++i) {
            auto interest = std::make_shared<ndn::Interest>(
                m_fileMetadata.getVersionedName().appendSegment(segmentNo));

            if (expressInterests(interest, &rxQueue) == 0) {
                break;
            } else {
                segmentNo += m_options.nthreads;
                ++nTx;
            }
        }

        if (m_stop) {
            return;
        }

        uint64_t nBytes = 0;

        for (auto i = 0; i < nTx && !m_stop; ++i) {
            ndn::Data data;
            if (!rxQueue.wait_dequeue_timed(data, m_options.lifetime.count() *
                                                      1000)) {
                // TODO: Handle timeout
                std::cout << "WARN: Request timeout for Interest";
                m_counters.nTimeout.fetch_add(1, std::memory_order_release);
                break;
            } else {
                m_counters.nData.fetch_add(1, std::memory_order_release);
                nBytes += data.getContent().value_size();
            }
        }

        onProgress(nBytes);
    }
}

int Runner::expressInterests(std::shared_ptr<ndn::Interest> interest,
                             RxQueue *rxQueue) {
    interest->setInterestLifetime(m_options.lifetime);

    if (!m_pipeline->enqueueInterestPacket(std::move(interest), rxQueue)) {
        std::cout << "FATAL: unable to enqueue Interest packet: "
                  << interest->getName() << "\n";
        m_stop = true;
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
