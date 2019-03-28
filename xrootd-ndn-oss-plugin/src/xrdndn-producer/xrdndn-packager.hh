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

#include <ndn-cxx/face.hpp>

namespace xrdndnproducer {
class Packager : public std::enable_shared_from_this<Packager> {
    static const ndn::security::SigningInfo signingInfo;
    static const std::shared_ptr<ndn::KeyChain> keyChain;

  public:
    Packager(uint64_t freshnessPeriod, bool disableSignature = false);
    ~Packager();

    const ndn::Data getPackage(ndn::Name &name, const int contentValue);
    const ndn::Data getPackage(ndn::Name &name, const uint8_t *value,
                               ssize_t size);

  private:
    void digest(ndn::Data &data);

  private:
    const ndn::time::milliseconds m_freshnessPeriod;

    bool m_disableSigning;
    ndn::Signature m_fakeSignature;
};
} // namespace xrdndnproducer

#endif // XRDNDN_PACKAGER