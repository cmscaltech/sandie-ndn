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

#ifndef XRDNDN_DFH_INTERFACE_HH
#define XRDNDN_DFH_INTERFACE_HH

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "xrdndn-namespaces.hh"

namespace xrdndn {

// System calls enumeration.
DEFINE_ENUM_WITH_STRING_CONVERSIONS(SystemCalls, (open)(read)(fstat)(close))

class FileHandlerInterface {
  public:
    virtual ~FileHandlerInterface() {}

  public:
    // File oriented methods

    /* Over NDN the file will be opened for reading only. On success returns 0;
     * on fail -1.*/
    virtual int Open(std::string path) = 0;

    /* Over NDN the name of the file to be close need to be passed. On success
     * returns 0; on fail -1.*/
    virtual int Close(std::string path) = 0;

    /* Over NDN the name of the files has to be passed. It returns 0 on success
     * and -1 on error. It fills buff with fstat result on file at path.*/
    virtual int Fstat(struct stat *buff, std::string path) = 0;

    /* It returns the number of bytes read from the file at path. Over NDN only
     * the actual data will be send and the return value will be computed on the
     * consumer side.*/
    virtual ssize_t Read(void *buff, off_t offset, size_t blen,
                         std::string path) = 0;
};
} // namespace xrdndn

#endif // XRDNDN_DFH_INTERFACE_HH