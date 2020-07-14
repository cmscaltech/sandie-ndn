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

#include "filesystem-hdfs.hh"

INIT_ZF_LOG(Xrdndndpdkfilesystem);

FileSystemHDFS::FileSystemHDFS() {
    struct hdfsBuilder *builder = hdfsNewBuilder();
    if (NULL == builder) {
        ZF_LOGF("Unable to allocate struct hdfsBuilder");
        throw "Unable to create HDFS File System object";
    }

    hdfsBuilderSetNameNode(builder, "default");
    m_hdfsFS = hdfsBuilderConnect(builder);

    if (NULL == m_hdfsFS) {
        ZF_LOGF("Unable to connect to HDFS");
        throw "Unable to create HDFS File System object";
    }
}

FileSystemHDFS::~FileSystemHDFS() {
    if (NULL != m_hdfsFS) {
        hdfsDisconnect(m_hdfsFS);
    }
}

int FileSystemHDFS::open(const char *pathname) {
    ZF_LOGI("Open file: %s", pathname);

    if (this->hasFileHandler(pathname)) {
        return FILESYSTEM_ESUCCESS;
    }

    hdfsFile file = hdfsOpenFile(m_hdfsFS, pathname, O_RDONLY, 0, 0, 0);
    if (NULL == file) {
        ZF_LOGW("Unable to open file: %s", pathname);
        return FILESYSTEM_EFAILURE;
    }

    if (unlikely(this->storeFileHandler(pathname, file) ==
                 FILESYSTEM_EFAILURE)) {
        hdfsCloseFile(m_hdfsFS, file);
        return FILESYSTEM_EFAILURE;
    }

    return FILESYSTEM_ESUCCESS;
}

void FileSystemHDFS::close(const char *pathname, std::any fd) {
    ZF_LOGD("Close file: %s", pathname);
    if (hdfsCloseFile(m_hdfsFS, std::any_cast<hdfsFile>(fd)) == -1) {
        ZF_LOGW("Closing file handler for file: %s with errcode: %d (%s)",
                pathname, errno, strerror(errno));
    }
}

int FileSystemHDFS::fstat(const char *pathname, void *buf) {
    ZF_LOGI("Fstat file: %s", pathname);

    hdfsFileInfo *fileInfo = hdfsGetPathInfo(m_hdfsFS, pathname);
    if (NULL == fileInfo) {
        ZF_LOGW("Unable to retrieve path info for file: %s", pathname);
        return FILESYSTEM_EFAILURE;
    }

    (reinterpret_cast<struct stat *>(buf))->st_mode = fileInfo->mPermissions;
    (reinterpret_cast<struct stat *>(buf))->st_mode |=
        (fileInfo->mKind == kObjectKindDirectory) ? S_IFDIR : S_IFREG;
    (reinterpret_cast<struct stat *>(buf))->st_nlink =
        (fileInfo->mKind == kObjectKindDirectory) ? 0 : 1;
    (reinterpret_cast<struct stat *>(buf))->st_uid = 1;
    (reinterpret_cast<struct stat *>(buf))->st_gid = 1;
    (reinterpret_cast<struct stat *>(buf))->st_size =
        (fileInfo->mKind == kObjectKindDirectory) ? 4096 : fileInfo->mSize;
    (reinterpret_cast<struct stat *>(buf))->st_mtime = fileInfo->mLastMod;
    (reinterpret_cast<struct stat *>(buf))->st_atime = fileInfo->mLastMod;
    (reinterpret_cast<struct stat *>(buf))->st_ctime = fileInfo->mLastMod;
    (reinterpret_cast<struct stat *>(buf))->st_dev = 0;
    (reinterpret_cast<struct stat *>(buf))->st_ino = 0;

    hdfsFreeFileInfo(fileInfo, 1);
    return FILESYSTEM_ESUCCESS;
}

int FileSystemHDFS::read(const char *pathname, void *buf, size_t count,
                         off_t offset) {
    ZF_LOGD("Read %zuB @%ld from file: %s", count, offset, pathname);

    auto file = this->getFileHandler(pathname);
    if (unlikely(!file.has_value())) {
        ZF_LOGI("File descriptor for: %s not available. Opening file",
                pathname);

        if (unlikely(this->open(pathname) != 0)) {
            return -(FILESYSTEM_EFAILURE);
        } else {
            file = this->getFileHandler(pathname);
        }
    }

    int ret =
        hdfsPread(m_hdfsFS, std::any_cast<hdfsFile>(file), offset, buf, count);
    if (unlikely(ret == -1)) {
        ZF_LOGW("Reading %zuB @%ld from file: %s failed with errcode: %d (%s)",
                count, offset, pathname, errno, strerror(errno));
        return -errno;
    }

    return ret;
}
