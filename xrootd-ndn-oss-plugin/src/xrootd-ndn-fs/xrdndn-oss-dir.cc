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

#include "xrdndn-oss-dir.hh"

XrdNdnOssDir::XrdNdnOssDir(const char *) {}

XrdNdnOssDir::~XrdNdnOssDir() {}

int XrdNdnOssDir::Opendir(const char *, XrdOucEnv &) { return -ENOTDIR; }

int XrdNdnOssDir::Readdir(char *, int) { return -ENOTDIR; }

int XrdNdnOssDir::StatRet(struct stat *) { return -ENOTSUP; }

int XrdNdnOssDir::Close(long long *) { return 0; }
