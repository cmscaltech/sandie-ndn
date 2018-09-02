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

#include "xrdndn-directory-file-handler.hh"
#include "xrdndn-namespaces.hh"

namespace xrdndn {
class Utils {
  public:
    // NDN Interest specific helper functions //

    // Returns Name prefix for a system call
    static ndn::Name interestPrefix(SystemCalls sc) {
        return ndn::Name(PLUGIN_INTEREST_PREFIX_URI).append(enumToString(sc));
    }
    // Returns Name for a give system call on a file at path
    static ndn::Name interestName(SystemCalls sc, std::string path) {
        return interestPrefix(sc).append(path);
    }

    // NDN Name specific helper functions //

    // Returns file path from the ndn::Name for a specific system call
    static std::string getFilePathFromName(ndn::Name name, SystemCalls sc) {
        std::string ret;

        switch (sc) {
        case SystemCalls::read: {
            size_t nPrefixSz = interestPrefix(sc).size();
            size_t nComponents = name.size() - nPrefixSz - 1;
            ret = name.getSubName(nPrefixSz, nComponents).toUri();
            break;
        }
        case SystemCalls::open:
        case SystemCalls::close:
        case SystemCalls::fstat:
        default:
            ret = name.getSubName(interestPrefix(sc).size(), ndn::Name::npos)
                      .toUri();
            break;
        }

        return ret;
    }

    // Returns segment number for read file system call specific request
    template <typename Packet>
    static uint64_t getSegmentFromPacket(const Packet &packet) {
        ndn::Name name = packet.getName();

        if (name.at(-1).isVersion()) {
            return name.at(-2).toSegment();
        }
        return name.at(-1).toSegment();
    }
};
} // namespace xrdndn

#endif // XRDNDN_UTILS_HH