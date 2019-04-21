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

#ifndef XRDNDN_OSS_HH
#define XRDNDN_OSS_HH

#include <XrdOss/XrdOss.hh>
#include <XrdOuc/XrdOucErrInfo.hh>
#include <XrdOuc/XrdOucStream.hh>
#include <XrdSfs/XrdSfsAio.hh>
#include <XrdSys/XrdSysError.hh>
#include <XrdSys/XrdSysLogger.hh>
#include <XrdSys/XrdSysPlugin.hh>

#include "../xrdndn-consumer/xrdndn-consumer-options.hh"

class XrdSysLogger;

class XrdNdnOss : public XrdOss {
  public:
    XrdNdnOss() : XrdOss() {}
    ~XrdNdnOss() {}

    void Configure(const char *parms);
    static int Emsg(const char *, XrdOucErrInfo &, int, const char *x,
                    const char *y = "");
    void Say(const char *, const char *x = "", const char *y = "",
             const char *z = "");

    XrdOssDF *newDir(const char *tident);
    XrdOssDF *newFile(const char *tident);
    int Chmod(const char *, mode_t, XrdOucEnv *);
    int Create(const char *, const char *, mode_t, XrdOucEnv &, int);
    int Init(XrdSysLogger *, const char *);
    int Mkdir(const char *, mode_t, int, XrdOucEnv *);
    int Remdir(const char *, int, XrdOucEnv *);
    int Rename(const char *, const char *, XrdOucEnv *, XrdOucEnv *);
    int Stat(const char *, struct stat *, int, XrdOucEnv *);
    int Truncate(const char *, unsigned long long, XrdOucEnv *);
    int Unlink(const char *, int, XrdOucEnv *);

  public:
    XrdSysError *m_eDest;
    struct xrdndnconsumer::Options m_consumerOptions;
};

#endif // XRDNDN_OSS_HH