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

#include <boost/noncopyable.hpp>
#include <ndn-cxx/face.hpp>

#include "xrdndn-file-handler.hh"
#include "xrdndn-thread-safe-umap.hh"

using boost::noncopyable;

namespace xrdndnproducer {
class Producer : noncopyable {
  public:
    Producer(ndn::Face &face);
    ~Producer();

  private:
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

    xrdndn::ThreadSafeUMap<std::string, std::shared_ptr<FileHandler>>
        m_FileHandlers;
    std::shared_ptr<Packager> m_packager;

    const ndn::RegisteredPrefixId *m_xrdndnPrefixId;
    const ndn::InterestFilterId *m_OpenFilterId;
    const ndn::InterestFilterId *m_CloseFilterId;
    const ndn::InterestFilterId *m_FstatFilterId;
    const ndn::InterestFilterId *m_ReadFilterId;

    void registerPrefix();
    void insertNewFileHandler(std::string path);
};

} // namespace xrdndnproducer

#endif // XRDNDN_PRODUCER_HH