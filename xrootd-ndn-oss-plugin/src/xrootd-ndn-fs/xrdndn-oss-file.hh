// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

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