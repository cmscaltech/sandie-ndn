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

#ifndef XRDNDN_PACKAGER
#define XRDNDN_PACKAGER

#include <boost/thread/shared_mutex.hpp>
#include <ndn-cxx/face.hpp>

namespace xrdndnproducer {
class Packager {
  public:
    Packager(uint32_t signerType);
    ~Packager();

    std::shared_ptr<ndn::Data> getPackage(const ndn::Name &name, int value);
    std::shared_ptr<ndn::Data> getPackage(ndn::Name &name, const uint8_t *value,
                                          ssize_t size);

  private:
    void digest(std::shared_ptr<ndn::Data> &data);

  private:
    ndn::KeyChain m_keyChain;
    ndn::security::SigningInfo::SignerType m_signerType;
    mutable boost::shared_mutex m_mutex;
};
} // namespace xrdndnproducer

#endif // XRDNDN_PACKAGER