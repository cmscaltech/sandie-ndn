/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2023 California Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef NDNC_LIB_XRD_NDN_OSS_HPP
#define NDNC_LIB_XRD_NDN_OSS_HPP

#include <XrdOss/XrdOss.hh>
#include <XrdOuc/XrdOucErrInfo.hh>
#include <XrdOuc/XrdOucStream.hh>
#include <XrdSfs/XrdSfsAio.hh>
#include <XrdSys/XrdSysError.hh>
#include <XrdSys/XrdSysLogger.hh>
#include <XrdSys/XrdSysPlugin.hh>

#include "lib/posix/consumer.hpp"

class XrdSysLogger;

namespace xrdndnofs {
class XrdNdnOss : public XrdOss {
  public:
    XrdNdnOss();
    ~XrdNdnOss();

  public:
    bool Config(const char *parms);
    void Say(const char *, const char *, const char *, const char *);

    /**
     * @brief Source:
     * https://github.com/opensciencegrid/xrootd-hdfs/blob/master/src/XrdHdfs.cc#L1283
     *
     * @param pfx Message prefix value
     * @param einfo Place to put text and error code
     * @param ecode The error code
     * @param op The operation being performed
     * @param target The target (e.g., fname)
     * @return int
     */
    static int Emsg(const char *, XrdOucErrInfo &, int, const char *x,
                    const char *y = "");

  public:
    XrdOssDF *newDir(const char *) final;
    XrdOssDF *newFile(const char *) final;
    int Chmod(const char *, mode_t, XrdOucEnv *) final;
    int Create(const char *, const char *, mode_t, XrdOucEnv &, int) final;
    int Init(XrdSysLogger *, const char *) final;
    int Mkdir(const char *, mode_t, int, XrdOucEnv *) final;
    int Remdir(const char *, int, XrdOucEnv *) final;
    int Rename(const char *, const char *, XrdOucEnv *, XrdOucEnv *) final;
    int Stat(const char *, struct stat *, int, XrdOucEnv *) final;
    int Truncate(const char *, unsigned long long, XrdOucEnv *) final;
    int Unlink(const char *, int, XrdOucEnv *) final;

  public:
    XrdOucErrInfo error_;
    XrdSysError *eDest_;

    struct ndnc::posix::ConsumerOptions options_;

  private:
    std::shared_ptr<ndnc::posix::Consumer> consumer_;
};
}; // namespace xrdndnofs

#endif // NDNC_LIB_XRD_NDN_OSS_HPP
