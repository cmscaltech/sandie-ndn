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

#ifndef XRDNDN_DATA_FETCHER_HH
#define XRDNDN_DATA_FETCHER_HH

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>

#include "../common/xrdndn-logger.hh"

namespace xrdndnconsumer {
/**
 * @brief This class implements an NDN Data Fetcher. It takes care of only one
 * Interest packet and handles Nack, Timeout or Data for the Interest packet
 *
 */
class DataFetcher {
    /**
     * @brief Maximum no. of retries on receiving Nack Duplicate/Congestion
     * before setting error
     *
     */
    static const uint8_t MAX_RETRIES_NACK;
    /**
     * @brief Maximum no. of retries on receiving Timeout before setting error
     *
     */
    static const uint8_t MAX_RETRIES_TIMEOUT;

    /**
     * @brief Backoff time in case of congestion before resending Interest
     * packet
     *
     */
    static const ndn::time::milliseconds MAX_CONGESTION_BACKOFF_TIME;

    using FailureCallback = std::function<void(const ndn::Interest &interest,
                                               const std::string &reason)>;

  public:
    /**
     * @brief Returns a pointer to DataFetcher object for an Interest packet and
     * expresses the Interest
     *
     * @param face Reference to NDN Face which provides a communication channel
     * with local or remote NDN forwarder
     * @param interest The Interest to be handled by this object
     * @param onData Callback function on received Data for Interest packet
     * @param onFailure Callback function when DataFetcher fails (no. of
     * maximum Nacks or Timeouts is reached)
     * @return std::shared_ptr<DataFetcher> Pointer to a new DataFetcher object
     * for a specific Interest packet
     */
    static std::shared_ptr<DataFetcher> fetch(ndn::Face &face,
                                              const ndn::Interest &interest,
                                              ndn::DataCallback onData,
                                              FailureCallback onFailure);
    /**
     * @brief Checks if the Interest packet is processed
     *
     * @return true The Specific Interet packet is currently processed
     * @return false The Specific Interet packet wasn't processed or an error
     * occured
     */
    bool isFetching();

    /**
     * @brief Will cancel the pending Interest packet and free all allocated
     * resources
     *
     */
    void stop();

  private:
    /**
     * @brief Construct a new Data Fetcher object
     *
     * @param face Reference to NDN Face which provides a communication channel
     * with local or remote NDN forwarder
     * @param onData Callback on receiving Data
     * @param onFailure Callback to handle error
     */
    DataFetcher(ndn::Face &face, ndn::DataCallback onData,
                FailureCallback onFailure);

    /**
     * @brief Method called when receiving Data for Interest packet
     *
     * @param interest The Interest packet
     * @param data The Data packet for the Interest packet
     */
    void handleData(const ndn::Interest &interest, const ndn::Data &data);

    /**
     * @brief Method called when receiving NACK for Interest packet
     *
     * @param interest The Interest packet
     * @param nack  Nack object for the Interest packet
     */
    void handleNack(const ndn::Interest &interest, const ndn::lp::Nack &nack);

    /**
     * @brief Method called when receiving Timeut for Interest packet
     *
     * @param interest The Interest packet
     */
    void handleTimeout(const ndn::Interest &interest);

    /**
     * @brief Express Interest on NDN Face
     *
     * @param interest The Interest packet to be expressed
     */
    void expressInterest(const ndn::Interest &interest);

  private:
    ndn::DataCallback m_onData;
    FailureCallback m_onFailure;

    ndn::Face &m_face;
    ndn::util::Scheduler m_scheduler;
    const ndn::PendingInterestId *m_interestId;

    uint8_t m_nNacks;
    uint8_t m_nCongestionRetries;
    uint8_t m_nTimeouts;

    bool m_error;
    bool m_stop;
};
} // namespace xrdndnconsumer

#endif // XRDNDN_DATA_FETCHER_HH