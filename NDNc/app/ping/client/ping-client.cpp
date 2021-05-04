/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2021 California Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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
Runner::Runner(Face &face, Options options) : PacketHandler(face), m_options{options}, m_counters{}, m_pitGenerator{} {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    m_sequence = dist(gen);
    m_next = ndn::time::system_clock::now() + m_options.interval;
}

void Runner::loop() {
    auto now = ndn::time::system_clock::now();
    if (m_next > now) {
        return;
    }

    if (this->sendInterest()) {
        m_next = now + m_options.interval;
    }
}

bool Runner::sendInterest() {
    auto name = ndn::Name(m_options.prefix);
    name.appendSegment(m_sequence);

    std::shared_ptr<ndn::Interest> interest = std::make_shared<ndn::Interest>();
    interest->setName(name);
    interest->setInterestLifetime(m_options.lifetime);

    if (expressInterest(interest, m_pitGenerator.getNext())) {
        m_pendingInterests[m_pitGenerator.getSequenceValue()] = ndn::time::system_clock::now();

        ++m_sequence;
        ++m_counters.nTxInterests;
        return true;
    }

    return false;
}

void Runner::processData(std::shared_ptr<ndn::Data> &data, ndn::lp::PitToken pitToken) {
    ++m_counters.nRxData;

    auto now = ndn::time::system_clock::now();
    auto seq = lp::PitTokenGenerator::getPitValue(pitToken);
    auto rtt = ndn::time::duration_cast<ndn::time::microseconds>(now - m_pendingInterests[seq]);

    std::cout << ndn::time::toString(ndn::time::system_clock::now()) << " "
              << boost::lexical_cast<std::string>(pitToken) << " " << data->getName() << "\t" << rtt << "\n";
}

void Runner::processNack(std::shared_ptr<ndn::lp::Nack> &nack) {
    ++m_counters.nRxNacks;
    std::cout << "WARN: Received NACK for Interest " << nack->getInterest().getName()
              << " with reason: " << nack->getReason() << "\n";
}

// TODO: Handle timeout

Runner::Counters Runner::readCounters() {
    return this->m_counters;
}
}; // namespace client
}; // namespace ping
}; // namespace ndnc
