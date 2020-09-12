// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef XRDNDN_PIPELINE_HH
#define XRDNDN_PIPELINE_HH

#include <atomic>
#include <unordered_map>

#include <ndn-cxx/face.hpp>

#include "../common/xrdndn-logger.hh"
#include "xrdndn-data-fetcher.hh"

namespace xrdndnconsumer {
/**
 * @brief This class implements a fixed window size pipeline. By using
 * DataFetcher class, Interest packets will be handled in a controlled manner
 *
 */
class Pipeline {
    using NotifyTaskCompleteSuccess = std::function<void(const ndn::Data &)>;
    using NotifyTaskCompleteFailure = std::function<void()>;

    using DataTypeTuple = std::tuple<int, ndn::Interest, ndn::Data>;
    using FutureType = std::future<DataTypeTuple>;

  public:
    /**
     * @brief Construct a new Fixed Window Size Pipeline object
     *
     * @param face Reference to NDN Face which provides a communication channel
     * with local or remote NDN forwarder
     * @param size Fixed window size of Pipeline. The maximum concurrent
     * Interest packets expressed at one time
     */
    Pipeline(ndn::Face &face, size_t size);

    /**
     * @brief Destroy the Pipeline object
     *
     */
    ~Pipeline();

    /**
     * @brief Stops Pipeline execution. All expressed Interests are canceled
     *
     */
    void stop();

    /**
     * @brief Insert a new Interest in Pipeline to be processed. When a slot is
     * available, the Interest will be expressed
     *
     * @param interest The Interest to be expressed
     * @return FutureType Tasks (DataFetcher) in Pipeline are asynchronous, but
     * the read of chunk from a file needs to be synchronous, thus the
     * syncronization is done by using std::futures. When the Data is available
     * for an expressed Interest, the Consumer will be notified and a slot will
     * become available in Pipeline
     */
    FutureType insert(const ndn::Interest &interest);

    /**
     * @brief Print throughput information
     *
     * @param path File processed while using this Pipeline instance object
     */
    void getStatistics(std::string path = "N/A");

  private:
    /**
     * @brief Callback function for when task in Pipeline - DataFetcher has Data
     * for Interest. When this is called, the Consumer already has the Data. The
     * DataFetcher object can be destroyed a new Interest can be processed in
     * Pipeline
     *
     * @param data Data for expressed Interest
     * @param pipeNo Task number in Pipeline. Used to keep track of it until
     * destruction
     */
    void onTaskCompleteSuccess(const ndn::Data &data, const uint64_t &pipeNo);

    /**
     * @brief Callback function when DataFetcher has a failure. The Pipeline
     * will stop processing new requests. The taks in Pipeline will be
     * completed, but the Consumer will exit with corresponding error code
     *
     */
    void onTaskCompleteFailure();

  private:
    ndn::Face &m_face;
    size_t m_size;

    std::unordered_map<uint64_t, std::shared_ptr<DataFetcher>> m_window;
    boost::condition_variable m_cvWindow;
    boost::mutex m_mtxWindow;

    std::atomic<bool> m_stop;
    std::atomic<uint64_t> m_pipeNo;
    std::atomic<uint64_t> m_nSegmentsReceived;
    std::atomic<uint64_t> m_nBytesReceived;

    ndn::time::steady_clock::TimePoint m_startTime;
    ndn::time::duration<double, ndn::time::milliseconds::period> m_duration;
};
} // namespace xrdndnconsumer

#endif // XRDNDN_PIPELINE_HH