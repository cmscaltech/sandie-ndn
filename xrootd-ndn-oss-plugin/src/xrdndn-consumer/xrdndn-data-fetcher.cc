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

#include <cmath>

#include <boost/lexical_cast.hpp>

#include "xrdndn-data-fetcher.hh"

using namespace ndn;

namespace xrdndnconsumer {
const uint8_t DataFetcher::MAX_RETRIES_NACK = 16;
const uint8_t DataFetcher::MAX_RETRIES_TIMEOUT = 32;
const ndn::time::milliseconds DataFetcher::MAX_CONGESTION_BACKOFF_TIME =
    ndn::time::seconds(8);

std::shared_ptr<DataFetcher> DataFetcher::fetch(Face &face,
                                                const Interest &interest,
                                                DataCallback onData,
                                                FailureCallback onFailure) {
    auto dataFetcher = std::shared_ptr<DataFetcher>(
        new DataFetcher(face, std::move(onData), std::move(onFailure)));

    dataFetcher->expressInterest(interest);
    return dataFetcher;
}

DataFetcher::DataFetcher(Face &face, DataCallback onData,
                         FailureCallback onFailure)
    : m_face(face), m_scheduler(face.getIoService()), m_nNacks(0),
      m_nCongestionRetries(0), m_nTimeouts(0), m_error(false), m_stop(false) {
    m_onData = std::move(onData);
    m_onFailure = std::move(onFailure);
}

bool DataFetcher::isFetching() { return !m_stop && !m_error; }

void DataFetcher::stop() {
    if (this->isFetching()) {
        m_stop = true;
        m_face.removePendingInterest(m_interestId);
        m_scheduler.cancelAllEvents();
    }
}

void DataFetcher::handleData(const Interest &interest, const Data &data) {
    if (!this->isFetching())
        return;

    m_stop = true;
    m_onData(interest, data);
}

void DataFetcher::handleNack(const Interest &interest, const lp::Nack &nack) {
    NDN_LOG_TRACE("Received NACK with reason: \""
                  << nack.getReason() << "\" for Interest " << interest);

    if (m_nNacks >= MAX_RETRIES_NACK) {
        NDN_LOG_ERROR("Reached the maximum number of NACK retries: "
                      << m_nNacks << " for Interest: " << interest);
        m_error = true;
        m_onFailure(-ENETUNREACH);
        return;
    } else {
        ++m_nNacks;
    }

    Interest newInterest(interest);
    newInterest.refreshNonce();

    switch (nack.getReason()) {
    case lp::NackReason::DUPLICATE: {
        this->expressInterest(newInterest);
        break;
    }
    case lp::NackReason::CONGESTION: {
        time::milliseconds backOffTime(
            static_cast<uint64_t>(std::pow(2, m_nCongestionRetries)));

        if (backOffTime > MAX_CONGESTION_BACKOFF_TIME)
            backOffTime = MAX_CONGESTION_BACKOFF_TIME;
        else
            ++m_nCongestionRetries;

        m_scheduler.scheduleEvent(
            backOffTime,
            bind(&DataFetcher::expressInterest, this, newInterest));
        break;
    }
    default:
        NDN_LOG_ERROR("NACK with reason " << nack.getReason()
                                          << " does not trigger a retry");
        m_error = true;
        m_onFailure(-ENETUNREACH);
        break;
    }
}

void DataFetcher::handleTimeout(const Interest &interest) {
    NDN_LOG_TRACE("Received TIMEOUT for Interest: " << interest);

    if (m_nTimeouts >= MAX_RETRIES_TIMEOUT) {
        NDN_LOG_ERROR("Reached the maximum number of timeout retries: "
                      << m_nTimeouts << " for Interest: " << interest);
        m_error = true;
        m_onFailure(-ETIMEDOUT);
        return;
    } else {
        ++m_nTimeouts;
    }

    Interest newInterest(interest);
    newInterest.refreshNonce();
    this->expressInterest(newInterest);
}

void DataFetcher::expressInterest(const Interest &interest) {
    NDN_LOG_TRACE("Express Interest: " << interest);

    m_nCongestionRetries = 0;
    m_interestId = m_face.expressInterest(
        interest, std::bind(&DataFetcher::handleData, this, _1, _2),
        std::bind(&DataFetcher::handleNack, this, _1, _2),
        std::bind(&DataFetcher::handleTimeout, this, _1));
}
} // namespace xrdndnconsumer