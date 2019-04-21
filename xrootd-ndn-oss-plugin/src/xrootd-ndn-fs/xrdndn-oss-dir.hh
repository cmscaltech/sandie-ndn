/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018-2019 California Institute of Technology                   *
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

#ifndef XRDNDN_OSS_DIR_HH
#define XRDNDN_OSS_DIR_HH

#include "xrdndn-oss.hh"

class XrdNdnOssDir : public XrdOssDF {
  public:
    XrdNdnOssDir(const char *);
    ~XrdNdnOssDir();

  public:
    int Opendir(const char *, XrdOucEnv &);
    int Readdir(char *, int);
    int StatRet(struct stat *);
    int Close(long long * = 0);
};

#endif // XRDNDN_OSS_DIR_HH