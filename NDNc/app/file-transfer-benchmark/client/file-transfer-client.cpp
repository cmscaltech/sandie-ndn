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

#include "file-transfer-client.hpp"

namespace ndnc {
namespace benchmark {
namespace fileTransferClient {
Runner::Runner(Face &face, Options options)
    : m_options(options), m_counters(), m_stop(false) {
    m_pipeline = new Pipeline(face);
}

Runner::~Runner() {
    m_stop = true;
    wait();

    if (NULL != m_pipeline) {
        delete m_pipeline;
    }
}

void Runner::run(NotifyProgressStatus onProgress) {
    for (auto i = 0; i < m_options.nThreads; ++i) {
        m_workers.push_back(
            std::thread(&Runner::transfer, this, i, onProgress));
    }
}

void Runner::wait() {
    for (auto i = 0; i < m_options.nThreads; ++i) {
        if (m_workers[i].joinable()) {
            m_workers[i].join();
        }
    }
}

void Runner::stop() {
    m_stop = true;
}

void Runner::transfer(int index, NotifyProgressStatus onProgress) {
    uint64_t offset = index * m_options.fileReadChunk;
    RxQueue rxQueue;

    while (!m_stop && m_pipeline->isValid() && offset < m_options.fileSize) {
        auto nPackets = expressInterests(offset, &rxQueue);
        if (nPackets == 0) {
            return;
        }

        uint64_t nBytes = 0;
        for (int i = 0; i < nPackets; ++i) {
            ndn::Data data;
            rxQueue.wait_dequeue(data);
            nBytes += data.getContent().size();
            m_counters.nData.fetch_add(1, std::memory_order_release);
        }

        offset += m_options.fileReadChunk * m_options.nThreads;
        onProgress(nBytes);
    }
}

int Runner::expressInterests(uint64_t offset, RxQueue *rxQueue) {
    auto firstSegment = offset / m_options.dataPayloadSize;
    auto lastSegment = ceil((offset + m_options.fileReadChunk) /
                            static_cast<double>(m_options.dataPayloadSize));

    for (auto i = firstSegment; i < lastSegment; ++i) {
        auto interest = std::make_shared<const ndn::Interest>(
            ndn::Name(m_options.prefix + m_options.filePath).appendSegment(i),
            m_options.interestLifetime);

        if (!m_pipeline->enqueueInterestPacket(std::move(interest), rxQueue)) {
            std::cout << "ERROR: unable to enqueue Interest packet\n";
            m_stop = true;
            return 0;
        }

        m_counters.nInterest.fetch_add(1, std::memory_order_release);
    }

    return lastSegment - firstSegment;
}

Runner::Counters &Runner::readCounters() {
    return this->m_counters;
}
}; // namespace fileTransferClient
}; // namespace benchmark
}; // namespace ndnc
