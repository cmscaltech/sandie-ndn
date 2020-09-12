// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

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