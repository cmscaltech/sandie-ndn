/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2019 California Institute of Technology                        *
 *                                                                            *
 * Author: Catalin Iordache <catalin.iordache@cern.ch>                        *
 *                                                                            *
 * This program is free software: you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.     *
 *****************************************************************************/

#include <iostream>

#include <ndn-cxx/util/time.hpp>

#include "../common/xrdndn-utils.hh"
#include "xrdndn-pipeline.hh"

using namespace ndn;

namespace xrdndnconsumer {
const uint8_t Pipeline::DEFAULT_PIPELINE_SIZE = 64;

Pipeline::Pipeline(Face &face, size_t size)
    : m_face(face), m_pipelineSz(size), m_nSegmentsReceived(0),
      m_nBytesReceived(0) {
    m_pipeline.resize(m_pipelineSz);
    m_startTime = ndn::time::steady_clock::now();
}

Pipeline::~Pipeline() { this->clear(); }

void Pipeline::clear() {
    for (auto &fetcher : m_pipeline) {
        if (fetcher != nullptr)
            fetcher->stop();
    }
    m_pipeline.clear();

    while (!m_pipelineQueue.empty()) {
        m_pipelineQueue.pop();
    }

    m_nSegmentsReceived = 0;
    m_nBytesReceived = 0;
}

void Pipeline::insert(const Interest &interest) {
    m_pipelineQueue.push(interest);
}

void Pipeline::run(ndn::DataCallback onData, FailureCallback onFailure) {
    if (m_pipelineQueue.empty()) {
        NDN_LOG_WARN("There are no Interest packets in Pipeline's queue.");
        return;
    }

    m_onData = std::move(onData);
    m_onFailure = std::move(onFailure);

    NDN_LOG_TRACE("Start processing the pipeline.");

    for (size_t i = 0; i < m_pipelineSz; ++i) {
        if (!fetchNextData(i))
            break;
    }
}

bool Pipeline::fetchNextData(size_t pipeNo) {
    if (m_pipelineQueue.empty()) {
        return false;
    }

    if (m_pipeline[pipeNo] && m_pipeline[pipeNo]->isFetching()) {
        NDN_LOG_ERROR(
            "Trying to replace an in-progress Interest from pipeline.");
        return false;
    }

    auto interest = m_pipelineQueue.front();
    m_pipelineQueue.pop();

    NDN_LOG_TRACE("Fetching Interest: " << interest);

    auto fetcher = DataFetcher::fetch(
        m_face, interest,
        std::bind(&Pipeline::handleData, this, _1, _2, pipeNo),
        std::bind(&Pipeline::handleFailure, this, _1, _2));

    m_pipeline[pipeNo] = fetcher;
    return true;
}

void Pipeline::handleData(const ndn::Interest &interest, const ndn::Data &data,
                          size_t pipeNo) {
    m_nSegmentsReceived++;
    m_nBytesReceived += data.getContent().value_size();

    m_onData(interest, data);
    fetchNextData(pipeNo);
}

void Pipeline::handleFailure(const ndn::Interest &interest,
                             const std::string &reason) {
    m_onFailure(interest, reason);
}

void Pipeline::printStatistics(std::string path) {
    ndn::time::duration<double, ndn::time::milliseconds::period> timeElapsed =
        ndn::time::steady_clock::now() - m_startTime;

    double throughput = (8 * m_nBytesReceived / 1000.0) / timeElapsed.count();

    std::cerr << "Transfer for file: "
                 << path << " over NDN completed."
                 << "\nTime elapsed: " << timeElapsed
                 << "\nTotal # of segments received: " << m_nSegmentsReceived
                 << "\nTotal size: "
                 << static_cast<double>(m_nBytesReceived) / 1000000 << " MB"
                 << "\nThroughput: " << throughput << " Mbit/s\n";
}
} // namespace xrdndnconsumer