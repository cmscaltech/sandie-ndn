/******************************************************************************
 * Named Data Networking plugin for XRootD                                    *
 * Copyright © 2019 California Institute of Technology                        *
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

#ifndef XRDNDNDPDK_UTILS_H
#define XRDNDNDPDK_UTILS_H

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ndn-dpdk/core/logger.h"
#include "ndn-dpdk/ndn/name.h"
#include "ndn-dpdk/ndn/nni.h"

#include "xrdndndpdk-namespace.h"

/**
 * @brief Empty LName structure
 *
 */
static const LName lnameStub = {.value = NULL, .length = 0};

/**
 * @brief Struct pointing to the Content in Data packet
 *
 */
typedef struct PContent {
    uint8_t
        type; // ContentType:
              // https://named-data.net/doc/NDN-packet-spec/current/data.html
    uint64_t length;  // Content length
    uint16_t offset;  // Content offset in Data packet
    uint8_t *payload; // Payload from Data packet
} PContent;

/**
 * @brief Get filepath Name component length
 *
 * @param name Packet Name as LName struct
 * @param off Offset to the first Name component of filepath. Skip prefix and
 * system call name
 * @return uint16_t Filepath Name component length
 */
uint16_t lnameGetFilePathLength(const LName name, uint16_t off);

/**
 * @brief Decode filepath from LName
 *
 * @param name Packet Name as LName struct
 * @param off Offset to the first Name component of filepath. Skip prefix and
 * system call name
 * @param filepath output
 * @return uint16_t offset to end of filepath in LName
 */
uint16_t lnameDecodeFilePath(const LName name, uint16_t off, char *filepath);

/**
 * @brief Get file system call id from Packet Name. See SystemCallId enum
 *
 * @param name Packet Name
 * @return SystemCallId File system call id
 */
SystemCallId lnameGetSystemCallId(const LName name);

/**
 * @brief Decode Content offset in Data NDN packet format v0.3
 * https://named-data.net/doc/NDN-packet-spec/current/types.html
 *
 * @param content PContent struct
 * @param len Length of payload
 */
void packetDecodeContent(PContent *content, uint16_t len);

/**
 * @brief Copy byte array from src to dst at a given offsets
 *
 * @param dst Destination buffer
 * @param dst_off Offset in dst buffer
 * @param src Source buffer
 * @param src_off Offset in src buffer
 * @param count Number of octets to copy from src to dst
 */
void copyFromC(uint8_t *dst, uint16_t dst_off, uint8_t *src, uint16_t src_off,
               uint64_t count);

#endif // XRDNDNDPDK_UTILS_H