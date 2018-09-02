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

#ifndef XRDNDN_PRODUCER_HH
#define XRDNDN_PRODUCER_HH

#include <fstream>
#include <ndn-cxx/face.hpp>

#include "xrdndn-directory-file-handler.hh"
#include "xrdndn-thread-safe-umap.hh"

namespace xrdndn {
class Producer : DFHandler {
  public:
    Producer(ndn::Face &face);
    ~Producer();

  private:
    virtual int Open(std::string path) override;
    void onOpenInterest(const ndn::InterestFilter &filter,
                        const ndn::Interest &interest);

    virtual int Close(std::string path) override;
    void onCloseInterest(const ndn::InterestFilter &filter,
                         const ndn::Interest &interest);

    virtual int Fstat(struct stat *buff, std::string path) override;
    void onFstatInterest(const ndn::InterestFilter &filter,
                         const ndn::Interest &interest);

    virtual ssize_t Read(void *buff, off_t offset, size_t blen,
                         std::string path) override;
    void onReadInterest(const ndn::InterestFilter &filter,
                        const ndn::Interest &interest);

  private:
    ndn::Face &m_face;
    ndn::KeyChain m_keyChain;

    const ndn::RegisteredPrefixId *m_xrdndnPrefixId;
    const ndn::InterestFilterId *m_OpenFilterId;
    const ndn::InterestFilterId *m_CloseFilterId;
    const ndn::InterestFilterId *m_FstatFilterId;
    const ndn::InterestFilterId *m_ReadFilterId;

    ThreadSafeUMap<std::string, std::shared_ptr<std::ifstream>>
        m_FileDescriptors;

    void registerPrefix();

    void send(std::shared_ptr<ndn::Data> data);
    void sendInteger(const ndn::Name &name, int value);
    void sendString(const ndn::Name &name, std::string buff, ssize_t size);
};

} // namespace xrdndn

#endif // XRDNDN_PRODUCER_HH