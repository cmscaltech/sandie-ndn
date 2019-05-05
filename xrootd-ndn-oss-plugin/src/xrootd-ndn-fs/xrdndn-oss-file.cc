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

#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include "xrdndn-oss-file.hh"

XrdNdnOssFile::XrdNdnOssFile(XrdNdnOss *xrdndnoss) : m_xrdndnoss(xrdndnoss) {}

XrdNdnOssFile::~XrdNdnOssFile() {}

/*****************************************************************************/
/*                                  O p e n                                  */
/*****************************************************************************/
// Over NDN the file will be opened for reading only
int XrdNdnOssFile::Open(const char *path, int, mode_t, XrdOucEnv &) {
    if (m_consumer != nullptr) {
        return -EINVAL;
    }

    m_consumer = xrdndnconsumer::Consumer::getXrdNdnConsumerInstance(
        m_xrdndnoss->m_consumerOptions);
    if (!m_consumer)
        return -EINVAL;

    int retOpen = m_consumer->Open(std::string(path));
    return retOpen;
}

/*****************************************************************************/
/*                                F s t a t                                  */
/*****************************************************************************/
int XrdNdnOssFile::Fstat(struct stat *buf) { return m_consumer->Fstat(buf); }

/*****************************************************************************/
/*                                  R e a d                                  */
/*****************************************************************************/
ssize_t XrdNdnOssFile::Read(void *buff, off_t offset, size_t blen) {
    int retRead = m_consumer->Read(buff, offset, blen);
    return retRead;
}

ssize_t XrdNdnOssFile::Read(off_t, size_t) { return XrdOssOK; }

ssize_t XrdNdnOssFile::ReadRaw(void *buff, off_t offset, size_t blen) {
    return Read(buff, offset, blen);
}

int XrdNdnOssFile::Read(XrdSfsAio *aiop) {
    aiop->Result = this->Read((void *)aiop->sfsAio.aio_buf,
                              aiop->sfsAio.aio_offset, aiop->sfsAio.aio_nbytes);
    aiop->doneRead();
    return 0;
}

/*****************************************************************************/
/*                                 C l o s e                                 */
/*****************************************************************************/
int XrdNdnOssFile::Close(long long *) {
    m_consumer->Close();
    return 0;
}

int XrdNdnOssFile::Fchmod(mode_t mode) {
    (void)mode;
    return -EISDIR;
}

int XrdNdnOssFile::Fsync() { return -EISDIR; }

int XrdNdnOssFile::Fsync(XrdSfsAio *aiop) {
    (void)aiop;
    return -EISDIR;
}

int XrdNdnOssFile::Ftruncate(unsigned long long) { return -EISDIR; }

int XrdNdnOssFile::getFD() { return -1; }

off_t XrdNdnOssFile::getMmap(void **addr) {
    (void)addr;
    return 0;
}

int XrdNdnOssFile::isCompressed(char *cxidp) {
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
