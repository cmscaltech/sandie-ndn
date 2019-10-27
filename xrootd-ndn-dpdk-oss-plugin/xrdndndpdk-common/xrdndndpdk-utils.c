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

#include "xrdndndpdk-utils.h"

INIT_ZF_LOG(Xrdndndpdkutils);

static uint64_t getLength(const uint8_t *buf, uint16_t *off) {
    ZF_LOGD("Get Name component length");
    uint64_t length = 0;

    if (likely(buf[*off] <= 0xFC)) {
        // Length value is encoded in this octet
        length |= buf[(*off)++];
    } else if (likely(buf[*off] == 0xFD)) {
        // Length value is encoded in the following 2 octets
        rte_be16_t v;
        rte_memcpy((uint8_t *)&v, &buf[++(*off)], sizeof(uint16_t));
        length = rte_be_to_cpu_16(v);
        *off += sizeof(uint16_t);

        assert(length > 0xFC);
    } else if (unlikely(buf[*off] == 0xFE)) {
        // Length value is encoded in the following 4 octets
        rte_be32_t v;
        rte_memcpy((uint8_t *)&v, &buf[++(*off)], sizeof(uint32_t));
        length = rte_be_to_cpu_32(v);
        *off += sizeof(uint32_t);

        assert(length > 0xFFFF);
    } else if (unlikely(buf[*off] == 0xFF)) {
        // Length value is encoded in the following 8 octets
        rte_be64_t v;
        rte_memcpy((uint8_t *)&v, &buf[++(*off)], sizeof(uint64_t));
        length = rte_be_to_cpu_64(v);
        *off += sizeof(uint64_t);

        assert(length > 0xFFFFFFFF);
    } else {
        ZF_LOGF("Length value encoding: %02X not found", buf[*off]);
        exit(XRDNDNDPDK_EFAILURE);
    }

    return length;
}

char *lnameGetFilePath(const LName name, uint16_t nameOff, bool hasSegmentNo) {
    ZF_LOGD("Get file path from name");

    char *path = rte_malloc(NULL, NAME_MAX_LENGTH * sizeof(char), 0);

    int16_t n = likely(hasSegmentNo) ? name.length - SEGMENT_NO_COMPONENT_SIZE
                                     : name.length;

    for (uint16_t off = nameOff, length = 0; off < n;) {
        assert(name.value[off] == TT_GenericNameComponent);
        ++off;
        length = getLength(name.value, &off);

        if (length + off > name.length && hasSegmentNo) {
            break;
        } else {
            strcat(path, "/");
            strncat(path, &name.value[off], length);
        }
        off += length;
    }

    return path;
}

uint64_t lnameGetSegmentNumber(const LName name) {
    ZF_LOGD("Get Segment Number from LName");

    rte_le64_t v;
    rte_memcpy((uint8_t *)&v, &name.value[name.length - sizeof(uint64_t)],
               sizeof(uint64_t));
    return rte_le_to_cpu_64(v);
}

SystemCallId lnameGetSystemCallId(const LName name) {
    ZF_LOGD("Get Systemcall ID from LName");

    if (likely(XRDNDNDPDK_SYCALL_PREFIX_READ_ENCODE_SIZE <= name.length &&
               memcmp(XRDNDNDPDK_SYCALL_PREFIX_READ_ENCODE, name.value,
                      XRDNDNDPDK_SYCALL_PREFIX_READ_ENCODE_SIZE) == 0)) {
        return SYSCALL_READ_ID;
    } else if (XRDNDNDPDK_SYCALL_PREFIX_OPEN_ENCODE_SIZE <= name.length &&
               memcmp(XRDNDNDPDK_SYCALL_PREFIX_OPEN_ENCODE, name.value,
                      XRDNDNDPDK_SYCALL_PREFIX_OPEN_ENCODE_SIZE) == 0) {
        return SYSCALL_OPEN_ID;
    } else if (XRDNDNDPDK_SYCALL_PREFIX_FSTAT_ENCODE_SIZE <= name.length &&
               memcmp(XRDNDNDPDK_SYCALL_PREFIX_FSTAT_ENCODE, name.value,
                      XRDNDNDPDK_SYCALL_PREFIX_FSTAT_ENCODE_SIZE) == 0) {
        return SYSCALL_FSTAT_ID;
    }

    return SYSCALL_NOT_FOUND;
}

PContent packetGetContent(uint8_t *packet, uint16_t len) {
    ZF_LOGD("Get Content from Packet");
    assert(packet[0] == TT_Data);

    uint8_t type = 0;
    uint64_t length = 0;
    uint16_t offset = 0;

    for (; offset < len;) {
        type = packet[offset++];
        assert(type == TT_Data || type == TT_Name || type == TT_MetaInfo ||
               type == TT_Content);

        length = getLength(packet, &offset);

        if (type == TT_Content)
            break; // Found Content in packet
        if (type != TT_Data) {
            offset += length; // Skip this type's length until Content
        }
    }

    PContent content = {
        .type = type, .length = length, .offset = offset, .buff = NULL};
    return content;
}
