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

#ifndef XRDNDN_UTILS_HH
#define XRDNDN_UTILS_HH

#include <cstring>
#include <ndn-cxx/face.hpp>

#include "xrdndn-directory-file-handler.hh"
#include "xrdndn-namespaces.hh"

namespace xrdndn {
class Utils {
  public:
    // NDN Interest specific helper functions.

    // Returns ndn::Name for a given system call
    static ndn::Name getInterestPrefix(SystemCalls sc) {
        return ndn::Name(PLUGIN_INTEREST_PREFIX_URI).append(enumToString(sc));
    }

    // Returns ndn::Name for a give system call on a file.
    static ndn::Name getInterestUri(SystemCalls sc, std::string path) {
        return getInterestPrefix(sc).append(path);
    }

    //  NDN Name specific helper functions

    // Returns file path from the ndn::Name for a specific system call
    static std::string getFilePathFromName(ndn::Name name, SystemCalls sc) {
        return name.getSubName(getInterestPrefix(sc).size(), -1).toUri();
    }
};
} // namespace xrdndn

#endif // XRDNDN_UTILS_HH