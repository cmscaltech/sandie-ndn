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

#ifndef XRDNDN_UTILS_HH
#define XRDNDN_UTILS_HH

#include <cstring>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/logger.hpp>

#include "xrdndn-dfh-interface.hh"
#include "xrdndn-namespaces.hh"

namespace xrdndn {
class Utils {
  public:
    // Given a SystemCall, it returns the Interest/Data Name prefix for it
    static ndn::Name interestPrefix(SystemCalls sc) noexcept {
        return ndn::Name(PLUGIN_INTEREST_PREFIX_URI).append(enumToString(sc));
    }
    // Given a file path and a SystemCall, it returns the Interest/Data Name
    // prefix for it
    static ndn::Name interestName(SystemCalls sc, std::string path,
                                  uint64_t segmentNo = 0) noexcept {
        auto name = interestPrefix(sc).append(path);

        if (sc == xrdndn::SystemCalls::read) {
            name.appendSegment(segmentNo);
        }
        return name;
    }

    // Returns file path from the ndn::Name for a SystemCall
    static std::string getFilePathFromName(ndn::Name name,
                                           SystemCalls sc) noexcept {
        auto subNameToUri = [name](ssize_t iStartComponent,
                                   size_t nComponents) {
            return name.getSubName(iStartComponent, nComponents).toUri();
        };

        auto prefixSz = interestPrefix(sc).size();
        auto nComponents = ndn::Name::npos;

        if (sc == SystemCalls::read) {
            nComponents = name.size() - prefixSz - 1;
        }

        return subNameToUri(prefixSz, nComponents);
    }

    // Returns segment number for read file system call specific request
    template <typename Packet>
    static uint64_t getSegmentFromPacket(const Packet &packet) noexcept {
        ndn::Name name = packet.getName();

        if (name.at(-1).isSegment())
            return name.at(-1).toSegment();

        if (name.at(-1).isVersion())
            return name.at(-2).toSegment();

        return 0;
    }
};
} // namespace xrdndn

#endif // XRDNDN_UTILS_HH