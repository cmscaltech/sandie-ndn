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

#include "filesystem-c-api.h"
#include "filesystem-posix.hh"

INIT_ZF_LOG(Xrdndndpdkfilesystem);

inline FileSystem *asFilesystem(void *obj) {
    return reinterpret_cast<FileSystem *>(obj);
}

void *libfs_newFilesystem(FilesystemType type) {
    void *obj = NULL;

    switch (type) {
    case FilesystemType::POSIX:
        ZF_LOGI("Get FileSystem object type: %d (POSIX)", type);
        obj = new FileSystemPosix();
        break;
    case FilesystemType::HADOOP:
        // ZF_LOGI("Get new FileSystem object type: %d (HADOOP)", type);
    case FilesystemType::CEPH:
        // ZF_LOGI("Get new FileSystem object type: %d (CEPH)", type);
    default:
        ZF_LOGE("Filesystem type: %d not supported", type);
    }

    ZF_LOGI("Return Filesystem object: %p", obj);
    return obj;
}

void libfs_destroyFilesystem(void *obj) {
    ZF_LOGI("Destroy Filesystem object: %p", obj);
    if (obj != nullptr) {
        // asFilesystem(obj)->~FileSystem(); // TODO
        delete asFilesystem(obj);
    }
}

int libfs_open(void *obj, const char *pathname) {
    return asFilesystem(obj)->open(pathname);
}

int libfs_fstat(void *obj, const char *pathname, void *buf) {
    return asFilesystem(obj)->fstat(pathname, buf);
}

int libfs_read(void *obj, const char *pathname, void *buf, size_t count,
               off_t offset) {
    return asFilesystem(obj)->read(pathname, buf, count, offset);
}