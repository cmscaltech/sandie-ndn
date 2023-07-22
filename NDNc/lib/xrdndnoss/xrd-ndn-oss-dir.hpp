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

#ifndef NDNC_LIB_XRD_NDN_OSS_DIR_HPP
#define NDNC_LIB_XRD_NDN_OSS_DIR_HPP

#include "lib/posix/dir.hpp"
#include "xrd-ndn-oss.hpp"

namespace xrdndnofs {
class XrdNdnOssDir : public XrdOssDF {
  public:
    XrdNdnOssDir(std::shared_ptr<ndnc::posix::Consumer> consumer);
    ~XrdNdnOssDir();

  public:
    int Opendir(const char *path, XrdOucEnv &);
    int Readdir(char *buff, int blen);
    int StatRet(struct stat *buff);
    int Close(long long *);

  private:
    std::shared_ptr<ndnc::posix::Consumer> consumer_;
    std::shared_ptr<ndnc::posix::Dir> dir_;
};
}; // namespace xrdndnofs

#endif // NDNC_LIB_XRD_NDN_OSS_DIR_HPP
