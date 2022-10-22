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

#include "xrd-ndn-oss-dir.hpp"

XrdNdnOssDir::XrdNdnOssDir(std::shared_ptr<ndnc::posix::Consumer> consumer) {
    this->dir_ = std::make_shared<ndnc::posix::Dir>(consumer);
}

XrdNdnOssDir::~XrdNdnOssDir() {
    this->Close(nullptr);
}

int XrdNdnOssDir::Opendir(const char *path, XrdOucEnv &) {
    return dir_->open(path);
}

int XrdNdnOssDir::Readdir(char *buff, int blen) {
    return dir_->read(buff, blen);
}

int XrdNdnOssDir::StatRet(struct stat *buff) {
    return dir_->stat(buff);
}

int XrdNdnOssDir::Close(long long * = 0) {
    return 0;
}
