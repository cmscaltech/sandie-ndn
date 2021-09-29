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
#include <random>

#include <boost/lexical_cast.hpp>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>

#include "ping-client.hpp"

namespace ndnc {
namespace ping {
namespace client {
Runner::Runner(Face &face, Options options)
    : m_options{options}, m_counters{}, m_stop(false) {

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    m_sequence = dist(gen);

    m_pipeline = new PipelineFixed(face, 1);
}

Runner::~Runner() {
    m_pipeline->stop();

    if (m_pipeline != nullptr) {
        delete m_pipeline;
    }
}

void Runner::stop() {
    m_stop = true;
    m_pipeline->stop();
}

void Runner::run() {
    auto interest = std::make_shared<ndn::Interest>(
        ndn::Name(m_options.name).appendSequenceNumber(++m_sequence));
    interest->setMustBeFresh(true);
    interest->setInterestLifetime(m_options.lifetime);

    RxQueue rxQueue;
    if (!m_pipeline->enqueueInterestPacket(std::move(interest), &rxQueue)) {
        std::cout << "WARN: unable to enqueue Interest packet\n";
        return;
    }

    auto start = ndn::time::system_clock::now();
    ++m_counters.nTxInterests;

    PendingInterestResult result;
    while (!rxQueue.wait_dequeue_timed(result, 1000)) {
        if (!m_pipeline->isValid()) {
            return;
        }
    }

    if (result.hasError() || m_stop) {
        return;
    }

    auto end = ndn::time::system_clock::now();
    ++m_counters.nRxData;

    auto rtt = ndn::time::duration_cast<ndn::time::microseconds>(end - start);
    std::cout << ndn::time::toString(end) << " " << result.getData()->getName()
              << " " << rtt << "\n";
}

Runner::Counters Runner::readCounters() {
    return m_counters;
}
}; // namespace client
}; // namespace ping
}; // namespace ndnc
