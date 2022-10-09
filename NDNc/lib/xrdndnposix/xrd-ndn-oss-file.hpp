/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2022 California Institute of Technology
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

#ifndef LIB_XRD_NDN_OSS_FILE_HPP
#define LIB_XRD_NDN_OSS_FILE_HPP

#include "xrd-ndn-oss.hpp"

class XrdSfsAio;

class XrdNdnOssFile : public XrdOssDF {
  public:
    XrdNdnOssFile();
    ~XrdNdnOssFile();

  public:
    int Fstat(struct stat *);
    int Open(const char *, int, mode_t, XrdOucEnv &);
    ssize_t Read(off_t, size_t);
    ssize_t Read(void *, off_t, size_t);
    int Read(XrdSfsAio *);
    ssize_t ReadRaw(void *, off_t, size_t);
    int Close(long long *);
    int Fchmod(mode_t);
    int Fsync();
    int Fsync(XrdSfsAio *);
    int Ftruncate(unsigned long long);
    int getFD();
    off_t getMmap(void **);
    int isCompressed(char *);
    ssize_t Write(const void *, off_t, size_t);
    int Write(XrdSfsAio *);
};

#endif // LIB_XRD_NDN_OSS_FILE_HPP
