// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
//
// Author: Catalin Iordache <catalin.iordache@cern.ch>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <ndn-cxx/util/time.hpp>

#include "../common/xrdndn-utils.hh"
#include "xrdndn-pipeline.hh"

using namespace ndn;

namespace xrdndnconsumer {
Pipeline::Pipeline(Face &face, size_t size)
    : m_face(face), m_size(size), m_stop(false), m_windowSize(0),
      m_nSegmentsReceived(0), m_nBytesReceived(0), m_duration(0) {
    NDN_LOG_TRACE("Alloc fixed window size " << m_size << " pipeline");
    m_startTime = ndn::time::steady_clock::now();
}

Pipeline::~Pipeline() {
    if (!m_window.empty()) {
        stop();
        m_window.clear();
    }
}

void Pipeline::stop() {
    m_stop = true;
    m_duration += ndn::time::steady_clock::now() - m_startTime;

    m_cvWindow.notify_all();
}

Pipeline::FutureType Pipeline::insert(const ndn::Interest &interest) {
    boost::unique_lock<boost::mutex> lock(m_mtxWindow);
    m_cvWindow.wait(lock, [&]() { return (m_windowSize < m_size) || m_stop; });

    if (m_stop) {
        NDN_LOG_TRACE("Pipeline will stop. Interest: "
                      << interest << " will not be processed");
        return FutureType();
    }

    auto fetcher = DataFetcher::getDataFetcher(
        m_face, interest, std::bind(&Pipeline::onTaskCompleteSuccess, this, _1),
        std::bind(&Pipeline::onTaskCompleteFailure, this));

    if (!fetcher) {
        NDN_LOG_ERROR(
            "Could not get DataFetcher object for Interest: " << interest);
        return FutureType();
    }

    NDN_LOG_TRACE("Processing Interest: " << interest);

    m_window.push_back(fetcher);
    fetcher->fetch();
    ++m_windowSize;

    return fetcher->get_future();
}

void Pipeline::onTaskCompleteSuccess(const ndn::Data &data) {
    m_nSegmentsReceived++;
    m_nBytesReceived += data.getContent().value_size();

    if (m_stop)
        return;

    m_windowSize--;
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