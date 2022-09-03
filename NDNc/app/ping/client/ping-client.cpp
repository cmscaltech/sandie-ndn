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

#include <random>

#include <boost/lexical_cast.hpp>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>

#include "logger/logger.hpp"
#include "ping-client.hpp"

namespace ndnc {
namespace ping {
Client::Client(face::Face &face, ClientOptions options)
    : m_options{options}, m_counters{}, m_stop(false) {

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    m_sequence = dist(gen);

    m_pipeline = std::make_shared<PipelineInterestsFixed>(face, 1);
}

Client::~Client() {
    this->stop();
}

void Client::stop() {
    m_stop = true;
    m_pipeline->close();
}

bool Client::canContinue() {
    return !m_stop && !m_pipeline->isClosed();
}

void Client::run() {
    auto interest = std::make_shared<ndn::Interest>(
        ndn::Name(m_options.name).appendSequenceNumber(++m_sequence));
    interest->setMustBeFresh(true);
    interest->setInterestLifetime(m_options.lifetime);

    if (!m_pipeline->pushInterest(0, std::move(interest))) {
        m_stop = true;
        LOG_WARN("unable to push Interest packet");
        return;
    }

    ++m_counters.nTxInterests;
    auto start = ndn::time::system_clock::now();

    std::shared_ptr<ndn::Data> data(nullptr);
    while (!m_stop && !m_pipeline->popData(0, data)) {}

    auto end = ndn::time::system_clock::now();

    if (m_stop || data == nullptr) {
        return;
    }

    auto rtt = ndn::time::duration_cast<ndn::time::microseconds>(end - start);
    LOG_INFO("%s %li us", data->getName().toUri().c_str(), rtt.count());

    ++m_counters.nRxData;
}

Client::Counters Client::getCounters() {
    return m_counters;
}
}; // namespace ping
}; // namespace ndnc
