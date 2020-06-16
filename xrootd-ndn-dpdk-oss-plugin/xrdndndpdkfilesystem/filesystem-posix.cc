/******************************************************************************
 * Named Data Networking plugin for XRootD                                    *
 * Copyright © 2019-2020 California Institute of Technology                   *
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

#include "filesystem-posix.hh"

INIT_ZF_LOG(Xrdndndpdkfilesystem);

FileSystemPosix::FileSystemPosix() {}

FileSystemPosix::~FileSystemPosix() {}

int FileSystemPosix::open(const char *pathname) {
    ZF_LOGI("Open file: %s", pathname);

    if (this->hasFileHandler(pathname)) {
        return FILESYSTEM_ESUCCESS;
    }

    int fd = ::open(pathname, O_RDONLY);
    if (fd == -1) {
        ZF_LOGW("Opening file: %s with errcode: %d (%s)", pathname, errno,
                strerror(errno));
        return errno;
    }

    if (unlikely(this->storeFileHandler(pathname, fd) == FILESYSTEM_EFAILURE)) {
        ::close(fd);
        return FILESYSTEM_EFAILURE;
    }

    return FILESYSTEM_ESUCCESS;
}

void FileSystemPosix::close(const char *pathname, std::any fd) {
    ZF_LOGD("Close file: %s", pathname);
    if (::close(std::any_cast<int>(fd)) == -1) {
        ZF_LOGW("Closing file handler for file: %s with errcode: %d (%s)",
                pathname, errno, strerror(errno));
    }
}

int FileSystemPosix::fstat(const char *pathname, void *buf) {
    ZF_LOGI("Fstat file: %s", pathname);

    if (::stat(pathname, reinterpret_cast<struct stat *>(buf)) == -1) {
        ZF_LOGW("Fstat on file: %s with errcode: %d (%s)", pathname, errno,
                strerror(errno));
        return errno;
    }
    return FILESYSTEM_ESUCCESS;
}

int FileSystemPosix::read(const char *pathname, void *buf, size_t count,
                          off_t offset) {
    ZF_LOGD("Read %zuB @%ld from file: %s", count, offset, pathname);

    auto fd = this->getFileHandler(pathname);
    if (unlikely(!fd.has_value())) {
        ZF_LOGI("File descriptor for: %s not available. Opening file",
                pathname);

        if (unlikely(this->open(pathname) != 0)) {
            return FILESYSTEM_EFAILURE;
        } else {
            fd = this->getFileHandler(pathname);
        }
    }

    int ret = pread(std::any_cast<int>(fd), buf, count, offset);
    if (unlikely(ret == -1)) {
        ZF_LOGW("Reading %zuB @%ld from file: %s failed with errcode: %d (%s)",
                count, offset, pathname, errno, strerror(errno));
        return -errno;
    }

    return ret;
}
