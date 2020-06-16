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

#pragma once

#ifndef XRDNDNDPDK_FILESYSTEM_C_API_H
#define XRDNDNDPDK_FILESYSTEM_C_API_H

#include <stddef.h>
#include <sys/types.h>

typedef enum { FT_POSIX = 0, FT_HDFS, FT_CEPHFS } FilesystemType;

#ifdef __cplusplus
extern "C" {
#endif

void *libfs_newFilesystem(FilesystemType type);
void libfs_destroyFilesystem(void *obj);

int libfs_open(void *obj, const char *pathname);
int libfs_fstat(void *obj, const char *pathname, void *buf);
int libfs_read(void *obj, const char *pathname, void *buf, size_t count,
               off_t offset);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // XRDNDNDPDK_FILESYSTEM_C_API_H
