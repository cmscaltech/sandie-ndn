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

#include "../../../ndn-dpdk/core/logger.h"
#include "../../../ndn-dpdk/ndn/name.h"

#include "xrdndndpdk-namespace.h"

#define SEGMENT_NO_COMPONENT_SIZE 10 // 1B Type, 1B Length, 8B sizeof(uint64_t)

/**
 * @brief Empty LName structure
 *
 */
static const LName lnameStub = {.value = "", .length = 0};

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
 * @brief Get file path from LName
 *
 * @param name LName struct
 * @param nameOff Offset of filepath in name. Skip prefix and system call name
 * @param hasSegmentNo Packet Name contains segment number
 * @return char* The path to the file
 */
char *lnameGetFilePath(const LName name, uint16_t nameOff, bool hasSegmentNo);

/**
 * @brief Get segment number (offset in file) from LName
 *
 * @param name Packet Name as LName struct
 * @return uint64_t The offset in file as uint64_t
 */
uint64_t lnameGetSegmentNumber(const LName name);

/**
 * @brief Get file system call id from Packet Name. See SystemCallId enum
 *
 * @param name Packet Name
 * @return SystemCallId File system call id
 */
SystemCallId lnameGetSystemCallId(const LName name);

/**
 * @brief Retrieve Content offset in Data NDN packet format v0.3
 * https://named-data.net/doc/NDN-packet-spec/current/types.html
 *
 * @param content PContent struct
 * @param len Length of payload
 */
void packetGetContent(PContent *content, uint16_t len);

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