/**************************************************************************
 * Named Data Networking plugin for xrootd                                *
 * Copyright © 2018 California Institute of Technology                    *
 *                                                                        *
 * Author: Catalin Iordache <catalin.iordache@cern.ch>                    *
 *                                                                        *
 * This program is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>. *
 **************************************************************************/

#ifndef XRDNDN_CONSUMER_HH
#define XRDNDN_CONSUMER_HH

#include <map>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/v2/validation-error.hpp>
#include <ndn-cxx/security/v2/validator.hpp>
#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>

#include "xrdndn-common.hh"
#include "xrdndn-directory-file-handler.hh"

namespace xrdndn {
static const uint8_t MAX_RETRIES = 32;
static const ndn::time::milliseconds CONGESTION_TIMEOUT =
    ndn::time::seconds(10);

class Consumer : DFHandler {
  public:
    Consumer();
    ~Consumer();

    virtual int Open(std::string path) override;
    virtual int Close(std::string path) override;
    virtual ssize_t Read(void *buff, off_t offset, size_t blen,
                         std::string path) override;

    static int getIntegerFromData(const ndn::Data &data) {
        int value = readNonNegativeInteger(data.getContent());
        return data.getContentType() == xrdndn::tlv::negativeInteger ? -value
                                                                     : value;
    }

  private:
    ndn::Face m_face;
    ndn::util::Scheduler m_scheduler;
    ndn::security::v2::Validator &m_validator;
    uint8_t m_nTimeouts;
    uint8_t m_nNacks;

    std::map<uint64_t, std::shared_ptr<const ndn::Data>> m_bufferedData;
    uint64_t m_nextToCopy;
    off_t m_buffOffset;

    int m_retOpen;
    int m_retClose;

    const ndn::Interest composeInterest(const ndn::Name name);
    void expressInterest(const ndn::Interest &interest, const SystemCalls call);

    void onNack(const ndn::Interest &interest, const ndn::lp::Nack &nack,
                const SystemCalls call);
    void onTimeout(const ndn::Interest &interest, const SystemCalls call);
    void onOpenData(const ndn::Interest &interest, const ndn::Data &data);
    void onCloseData(const ndn::Interest &interest, const ndn::Data &data);
    void onReadData(const ndn::Interest &interest, const ndn::Data &data);

    void saveDataInOrder(void *buff, off_t offset, size_t blen);
};
} // namespace xrdndn

#endif // XRDNDN_CONSUMER_HH