/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018-2019 California Institute of Technology                   *
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

#include "xrdndn-data-fetcher.hh"

using namespace ndn;

namespace xrdndnconsumer {
const uint8_t DataFetcher::MAX_RETRIES_NACK = 16;
const uint8_t DataFetcher::MAX_RETRIES_TIMEOUT = 32;
const ndn::time::milliseconds DataFetcher::MAX_CONGESTION_BACKOFF_TIME =
    ndn::time::seconds(8);

std::shared_ptr<DataFetcher>
DataFetcher::getDataFetcher(Face &face, const Interest &interest,
                            NotifyTaskCompleteSuccess onSuccess,
                            NotifyTaskCompleteFailure onFailure) {
    auto dataFetcher =
        std::make_shared<DataFetcher>(face, interest, onSuccess, onFailure);
    return dataFetcher;
}

DataFetcher::DataFetcher(ndn::Face &face, const ndn::Interest &interest,
                         NotifyTaskCompleteSuccess onSuccess,
                         NotifyTaskCompleteFailure onFailure)
    : m_face(face), m_scheduler(face.getIoService()), m_interest(interest),
      m_nNacks(0), m_nCongestionRetries(0), m_nTimeouts(0), m_error(false),
      m_stop(false) {
    m_onSuccess = std::move(onSuccess);
    m_onFailure = std::move(onFailure);

    m_task =
        TaskType(std::bind(&DataFetcher::onFutureCallback, this, _1, _2, _3));
}

void DataFetcher::stop() {
    if (this->isFetching()) {
        m_stop = true;
        m_interestId.cancel();
        m_scheduler.cancelAllEvents();
        m_task(-ECANCELED, m_interest, ndn::Data());
    }
}

void DataFetcher::fetch() { expressInterest(m_interest); }

bool DataFetcher::isFetching() { return !m_stop && !m_error; }

DataFetcher::FutureType DataFetcher::get_future() {
    return m_task.get_future();
}

DataFetcher::DataTypeTuple
DataFetcher::onFutureCallback(int errcode, const ndn::Interest &interest,
                              const ndn::Data &data) {
    return std::make_tuple(errcode, interest, data);
}

void DataFetcher::handleData(const Interest &interest, const Data &data) {
    if (!this->isFetching())
        return;

    NDN_LOG_TRACE("DataFetcher received Data for Interest: " << interest);

    m_stop = true;
    m_task(0, interest, data);
    m_onSuccess(data);
}

void DataFetcher::handleNack(const Interest &interest, const lp::Nack &nack) {
    NDN_LOG_TRACE("Received NACK with reason: \""
                  << nack.getReason() << "\" for Interest " << interest);

    if (m_nNacks >= MAX_RETRIES_NACK) {
        NDN_LOG_ERROR("Reached the maximum number of NACK retries: "
                      << m_nNacks << " for Interest: " << interest);
        m_error = true;
        m_task(-ENETUNREACH, interest, ndn::Data());
        m_onFailure();
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

        m_scheduler.schedule(backOffTime, bind(&DataFetcher::expressInterest,
                                               this, newInterest));
        break;
    }
    default:
        NDN_LOG_ERROR("NACK with reason " << nack.getReason()
                                          << " does not trigger a retry");
        m_error = true;
        m_task(-ENETUNREACH, interest, ndn::Data());
        m_onFailure();
        break;
    }
}

void DataFetcher::handleTimeout(const Interest &interest) {
    NDN_LOG_TRACE("Received TIMEOUT for Interest: " << interest);

    if (m_nTimeouts >= MAX_RETRIES_TIMEOUT) {
        NDN_LOG_ERROR("Reached the maximum number of timeout retries: "
                      << m_nTimeouts << " for Interest: " << interest);
        m_error = true;
        m_task(-ETIMEDOUT, interest, ndn::Data());
        m_onFailure();
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
    try {
        m_interestId = m_face.expressInterest(
            interest, std::bind(&DataFetcher::handleData, this, _1, _2),
            std::bind(&DataFetcher::handleNack, this, _1, _2),
            std::bind(&DataFetcher::handleTimeout, this, _1));
    } catch (const std::exception &e) {
        NDN_LOG_ERROR("Catch exception: " << e.what()
                                          << " while expressing Interest");
    }
}
} // namespace xrdndnconsumer