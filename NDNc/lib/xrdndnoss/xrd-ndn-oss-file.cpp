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

#include "xrd-ndn-oss-file.hpp"

namespace xrdndnofs {
XrdNdnOssFile::XrdNdnOssFile(std::shared_ptr<ndnc::posix::Consumer> consumer) {
    this->file_ = std::make_shared<ndnc::posix::File>(consumer);
}

XrdNdnOssFile::~XrdNdnOssFile() {
    this->Close(0);
}

int XrdNdnOssFile::Fstat(struct stat *buf) {
    if (this->file_ == nullptr) {
        return -EINVAL;
    }

    return file_->fstat(buf);
}

int XrdNdnOssFile::Open(const char *path, int, mode_t, XrdOucEnv &) {
    if (this->file_ == nullptr) {
        return -EINVAL;
    }

    return file_->open(path);
}

ssize_t XrdNdnOssFile::Read(off_t, size_t) {
    return XrdOssOK;
}

ssize_t XrdNdnOssFile::Read(void *buff, off_t offset, size_t blen) {
    if (file_ == nullptr) {
        return -EINVAL;
    }

    return file_->read(buff, offset, blen);
}

int XrdNdnOssFile::Read(XrdSfsAio *aoip) {
    aoip->Result = Read((void *)aoip->sfsAio.aio_buf, aoip->sfsAio.aio_offset,
                        aoip->sfsAio.aio_nbytes);
    aoip->doneRead();
    return 0;
}

ssize_t XrdNdnOssFile::ReadRaw(void *buff, off_t offset, size_t blen) {
    return Read(buff, offset, blen);
}

int XrdNdnOssFile::Close(long long * = 0) {
    if (file_ == nullptr) {
        return 0;
    }

    return file_->close();
}

int XrdNdnOssFile::Fchmod(mode_t mode) {
    (void)mode;
    return -EISDIR;
}

int XrdNdnOssFile::Fsync() {
    return -EISDIR;
}

int XrdNdnOssFile::Fsync(XrdSfsAio *aiop) {
    (void)aiop;
    return -EISDIR;
}

int XrdNdnOssFile::Ftruncate(unsigned long long) {
    return -EISDIR;
}

int XrdNdnOssFile::getFD() {
    return -1;
}

off_t XrdNdnOssFile::getMmap(void **addr) {
    (void)addr;
    return 0;
}

int XrdNdnOssFile::isCompressed(char *cxidp = 0) {
    (void)cxidp;
    return -EISDIR;
}

ssize_t XrdNdnOssFile::Write(const void *, off_t, size_t) {
    return (ssize_t)-EISDIR;
}

int XrdNdnOssFile::Write(XrdSfsAio *aiop) {
    (void)aiop;
    return (ssize_t)-EISDIR;
}
}; // namespace xrdndnofs
