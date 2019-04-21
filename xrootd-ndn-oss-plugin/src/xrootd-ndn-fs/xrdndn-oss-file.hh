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

#ifndef XRDNDN_OSS_FILE_HH
#define XRDNDN_OSS_FILE_HH

#include "../xrdndn-consumer/xrdndn-consumer.hh"
#include "xrdndn-oss.hh"

class XrdSfsAio;

class XrdNdnOssFile : public XrdOssDF {
  public:
    XrdNdnOssFile(XrdNdnOss *xrdndnoss);
    ~XrdNdnOssFile();

  public:
    // Supported file system calls

    int Fstat(struct stat *);
    int Open(const char *, int, mode_t, XrdOucEnv &);
    ssize_t Read(off_t, size_t);
    ssize_t Read(void *, off_t, size_t);
    int Read(XrdSfsAio *aoip);
    ssize_t ReadRaw(void *, off_t, size_t);
    int Close(long long *retsz = 0);

  public:
    int Fchmod(mode_t mode);
    int Fsync();
    int Fsync(XrdSfsAio *aiop);
    int Ftruncate(unsigned long long);
    int getFD();
    off_t getMmap(void **addr);
    int isCompressed(char *cxidp = 0);
    ssize_t Write(const void *, off_t, size_t);
    int Write(XrdSfsAio *aiop);

  private:
    std::shared_ptr<xrdndnconsumer::Consumer> m_consumer;
    XrdNdnOss *m_xrdndnoss;
};

#endif // XRDNDN_OSS_FILE_HH