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

#include <ndn-cxx/util/time.hpp>

#include "../common/xrdndn-utils.hh"
#include "xrdndn-pipeline.hh"

using namespace ndn;

namespace xrdndnconsumer {
Pipeline::Pipeline(Face &face, size_t size)
    : m_face(face), m_size(size), m_stop(false), m_nSegmentsReceived(0),
      m_nBytesReceived(0), m_pipeNo(0), m_duration(0) {
    NDN_LOG_TRACE("Alloc fixed window size " << m_size << " pipeline");
    m_startTime = ndn::time::steady_clock::now();
}

Pipeline::~Pipeline() {
    stop();
    m_window.clear();
}

void Pipeline::stop() {
    m_stop = true;
    m_duration += ndn::time::steady_clock::now() - m_startTime;

    {
        boost::unique_lock<boost::mutex> lock(m_mtxWindow);
        for (auto it = m_window.begin(); it != m_window.end(); ++it) {
            if (it->second->isFetching())
                it->second->stop();
        }
    }
    m_cvWindow.notify_all();
}

Pipeline::FutureType Pipeline::insert(const ndn::Interest &interest) {
    boost::unique_lock<boost::mutex> lock(m_mtxWindow);
    m_cvWindow.wait(lock,
                    [&]() { return (m_window.size() < m_size) || m_stop; });

    if (m_stop) {
        NDN_LOG_TRACE("Pipeline will stop. Interest: "
                      << interest << " will not be processed");
        return FutureType();
    }

    uint64_t pipeNo = m_pipeNo++;
    auto fetcher = DataFetcher::getDataFetcher(
        m_face, interest,
        std::bind(&Pipeline::onTaskCompleteSuccess, this, _1, pipeNo),
        std::bind(&Pipeline::onTaskCompleteFailure, this));

    if (!fetcher) {
        NDN_LOG_ERROR(
            "Could not get DataFetcher object for Interest: " << interest);
        return FutureType();
    }

    NDN_LOG_TRACE("Processing Interest: " << interest);
    m_window.emplace(
        std::pair<uint64_t, std::shared_ptr<DataFetcher>>(pipeNo, fetcher));
    auto future = m_window[pipeNo]->get_future();
    m_window[pipeNo]->fetch();

    return future;
}

void Pipeline::onTaskCompleteSuccess(const ndn::Data &data,
                                     const uint64_t &pipeNo) {
    m_nSegmentsReceived++;
    m_nBytesReceived += data.getContent().value_size();

    if (m_stop)
        return;

    {
        boost::unique_lock<boost::mutex> lock(m_mtxWindow);
        m_window.erase(pipeNo);

        if (m_window.empty()) {
            m_duration += ndn::time::steady_clock::now() - m_startTime;
            m_startTime = ndn::time::steady_clock::now();
        }
    }
    m_cvWindow.notify_one();
}

void Pipeline::onTaskCompleteFailure() {
    if (m_stop) {
        return;
    }

    NDN_LOG_ERROR("Pipeline task failed. New Interests will not be processed");
    m_stop = true;
    m_cvWindow.notify_all();
}

void Pipeline::getStatistics(std::string path) {
    double throughput = (8 * m_nBytesReceived / 1000.0) / m_duration.count();

    std::cout << "Transfer over NDN for file: " << path << " completed"
              << "\nTime elapsed: " << m_duration
              << "\nTotal # of segments received: " << m_nSegmentsReceived
              << "\nTotal size: "
              << static_cast<double>(m_nBytesReceived) / 1000000 << " MB"
              << "\nThroughput: " << throughput << " Mbit/s\n";
}
} // namespace xrdndnconsumer