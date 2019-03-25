/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018 California Institute of Technology                        *
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

#ifndef XRDNDN_PRODUCER_HH
#define XRDNDN_PRODUCER_HH

#include <ndn-cxx/face.hpp>

#include <boost/noncopyable.hpp>

#include "xrdndn-interest-manager.hh"
#include "xrdndn-producer-options.hh"

namespace xrdndnproducer {
class Producer : boost::noncopyable {
  public:
    static std::shared_ptr<Producer>
    getXrdNdnProducerInstance(ndn::Face &face, const Options &opts);

    Producer(ndn::Face &face, const Options &opts);
    ~Producer();

  private:
    void registerPrefix();
    void onData(const ndn::Data &data);

    void onOpenInterest(const ndn::InterestFilter &,
                        const ndn::Interest &interest);

    void onCloseInterest(const ndn::InterestFilter &,
                         const ndn::Interest &interest);

    void onFstatInterest(const ndn::InterestFilter &,
                         const ndn::Interest &interest);

    void onReadInterest(const ndn::InterestFilter &,
                        const ndn::Interest &interest);

  private:
    ndn::Face &m_face;
    bool m_error;

    const ndn::RegisteredPrefixId *m_xrdndnPrefixId;
    const ndn::InterestFilterId *m_OpenFilterId;
    const ndn::InterestFilterId *m_CloseFilterId;
    const ndn::InterestFilterId *m_FstatFilterId;
    const ndn::InterestFilterId *m_ReadFilterId;

    std::shared_ptr<InterestManager> m_interestManager;
};

} // namespace xrdndnproducer

#endif // XRDNDN_PRODUCER_HH