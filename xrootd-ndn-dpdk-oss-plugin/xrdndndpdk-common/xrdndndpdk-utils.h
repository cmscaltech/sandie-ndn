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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../../../ndn-dpdk/ndn/name.h"

#include "xrdndndpdk-namespace.h"

/**
 * @brief Empty LName structure
 *
 */
static const LName lnameStub = {.value = "", .length = 0};

/**
 * @brief Structer pointing to Content in Data packet
 *
 */
typedef struct PContent {
    uint8_t
        type; // ContentType:
              // https://named-data.net/doc/NDN-packet-spec/current/data.html
    uint64_t length; // Content length
    uint16_t offset; // Content offset in Data packet

    uint8_t *buff; // Content bytes of length
} PContent;

/**
 * @brief Parsing LName structure and retrieving the file path from Name
 *
 * @param lname LName structure
 * @param lnameOff Offset of filepath in name. Skipping prefix and system call
 * name
 * @param hasSegmentNo Name has segment number in it
 * @return char* The file path
 */
char *lnameGetFilePath(const LName lname, uint16_t lnameOff, bool hasSegmentNo);

/**
 * @brief Get file system call id from Packet Name
 *
 * @param name Packet Name
 * @return SystemCallId File system call id
 */
SystemCallId lnameGetSystemCallId(const LName name);

PContent packetGetContent(uint8_t *packet, uint16_t len);

// TODO: lnameGetSegmentNumber()

#endif // XRDNDNDPDK_UTILS_H