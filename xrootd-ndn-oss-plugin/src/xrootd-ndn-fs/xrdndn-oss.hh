// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

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