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

#include <fcntl.h>
#include <iostream>
#include <unistd.h>

#include <ndn-cxx/util/time.hpp>

static const ndn::time::milliseconds CLOSING_FILE_DELAY =
    ndn::time::seconds(180);

class FileDescriptor {
  public:
    FileDescriptor(const char *filePath) { m_fd = open(filePath, O_RDONLY); }

    ~FileDescriptor() {
        if (m_fd == -1)
            close(m_fd);
    }

    int get() { return m_fd; }

  private:
    int m_fd;
};