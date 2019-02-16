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

#ifndef XRDNDN_PIPELINE_HH
#define XRDNDN_PIPELINE_HH

#include <queue>

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
    /**
     * @brief Default fixed window size of pipeline. The maximum concurrent
     * Interest packets that are beeing expressed at a time in Pipeline
     *
     */
    static const uint8_t DEFAULT_PIPELINE_SIZE;

    using FailureCallback = std::function<void(const ndn::Interest &interest,
                                               const std::string &reason)>;

  public:
    /**
     * @brief Construct a new Fixed Window Size Pipeline object
     *
     * @param face Reference to NDN Face which provides a communication channel
     * with local or remote NDN forwarder
     * @param size Pipeline size if default size is not desired
     */
    Pipeline(ndn::Face &face, size_t size = DEFAULT_PIPELINE_SIZE);

    /**
     * @brief Destroy the Pipeline object
     *
     */
    ~Pipeline();

    /**
     * @brief Queue an Interest packet to be processed in pipeline
     *
     * @param interest The Interest to be queued
     */
    void insert(const ndn::Interest &interest);

    /**
     * @brief Start process the current queue of Interest packets
     *
     * @param onData Callback on receiving Data
     * @param onFailure Callback when an error occured while processing an
     * Interest packet
     */
    void run(ndn::DataCallback onData, FailureCallback onFailure);

    /**
     * @brief Clear all members of Pipeline object
     *
     */
    void clear();

    /**
     * @brief Print throughput information acquired while processing a number of
     * Interest packets in a period of time by Pipeline object
     *
     * @param path File processed while using a Pipeline instance object
     */
    void printStatistics(std::string path = "");

  private:
    /**
     * @brief Process one Interest packet from queue
     *
     * @param pipeNo The available spot in pipeline for next Interest packet
     * @return true Interest packet has successfully been expressed
     * @return false Error occured while trying to retrieve Data for Interest
     */
    bool fetchNextData(size_t pipeNo);

    /**
     * @brief Callback function when DataFetcher has aquired Data for Interest
     *
     * @param interest The Interest packet that was processed
     * @param data Data for the processed Interest
     * @param pipeNo The pipeline spot where the Interest was processed and
     * which just got available for next Interest in queue
     */
    void handleData(const ndn::Interest &interest, const ndn::Data &data,
                    size_t pipeNo);

    /**
     * @brief Callback function when DataFetcher has a failure
     *
     * @param interest The Interest that is presenting a failure
     * @param reason The reason of failure
     */
    void handleFailure(const ndn::Interest &interest,
                       const std::string &reason);

  private:
    ndn::Face &m_face;
    ndn::DataCallback m_onData;
    FailureCallback m_onFailure;

    std::queue<ndn::Interest> m_pipelineQueue;
    std::vector<std::shared_ptr<DataFetcher>> m_pipeline;

    size_t m_pipelineSz;

    ndn::time::steady_clock::TimePoint m_startTime;

    uint64_t m_nSegmentsReceived;
    uint64_t m_nBytesReceived;
};
} // namespace xrdndnconsumer

#endif // XRDNDN_PIPELINE_HH