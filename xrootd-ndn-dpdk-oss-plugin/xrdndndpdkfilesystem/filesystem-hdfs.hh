/******************************************************************************
 * Named Data Networking plugin for XRootD                                    *
 * Copyright © 2019-2020 California Institute of Technology                   *
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

#pragma once

#ifndef XRDNDNDPDK_FILESYSTEM_HDFS_HH
#define XRDNDNDPDK_FILESYSTEM_HDFS_HH

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "filesystem.hh"
#include "hdfs.h"

class FileSystemHDFS : public FileSystem {
  public:
    FileSystemHDFS();
    ~FileSystemHDFS();

    int open(const char *pathname);
    int fstat(const char *pathname, void *buf);
    int read(const char *pathname, void *buf, size_t count = 0,
             off_t offset = 0);
    void close(const char *pathname, std::any fd);

  private:
    hdfsFS m_hdfsFS;
};

#endif // XRDNDNDPDK_FILESYSTEM_HDFS_HH