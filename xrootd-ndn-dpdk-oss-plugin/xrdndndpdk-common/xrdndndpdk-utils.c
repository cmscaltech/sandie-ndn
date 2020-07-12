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

static uint64_t getGenericNameComponentLength(const uint8_t *buf,
                                              uint16_t *off) {
    ZF_LOGD("Get Name component length");
    uint64_t length = 0;

    if (buf[*off] <= 0xFC) {
        // Length value is encoded in this octet
        length |= buf[(*off)++];
    } else if (buf[*off] == 0xFD) {
        // Length value is encoded in the following 2 octets
        rte_be16_t v;
        rte_memcpy((uint8_t *)&v, &buf[++(*off)], sizeof(uint16_t));
        length = rte_be_to_cpu_16(v);
        *off += sizeof(uint16_t);

        assert(length > 0xFC);
    } else if (buf[*off] == 0xFE) {
        // Length value is encoded in the following 4 octets
        rte_be32_t v;
        rte_memcpy((uint8_t *)&v, &buf[++(*off)], sizeof(uint32_t));
        length = rte_be_to_cpu_32(v);
        *off += sizeof(uint32_t);

        assert(length > 0xFFFF);
    } else if (buf[*off] == 0xFF) {
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

uint16_t lnameGetFilePathLength(const LName name, uint16_t off) {
    ZF_LOGD("Get filepath Name component length");
    assert(name.value[off] == TT_GenericNameComponent);

    while (name.value[off] == TT_GenericNameComponent && off < name.length) {
        ++off;
        uint64_t length = getGenericNameComponentLength(name.value, &off);
        off += length;
    }
    return off;
}

uint16_t lnameDecodeFilePath(const LName name, uint16_t off, char *filepath) {
    ZF_LOGD("Decode filepath from LName");
    assert(name.value[off] == TT_GenericNameComponent);

    while (name.value[off] == TT_GenericNameComponent && off < name.length) {
        ++off; // skip Type
        uint64_t length =
            getGenericNameComponentLength(name.value, &off); // get Length
        strcat(filepath, "/");
        strncat(filepath, &name.value[off], length); // store Value
        off += length;
    }
    return off;
}

uint64_t lnameDecodeSegmentNumber(const LName name, uint16_t off) {
    ZF_LOGD("Decode segment number from LName");
    assert(name.value[off] == TT_SegmentNameComponent);

    uint64_t segNo = 0;
    DecodeNni(name.value[off + 1], &name.value[off + 2], &segNo);
    return segNo;
}

SystemCallId lnameDecodeSystemCallId(const LName name) {
    ZF_LOGD("Decode Systemcall ID from LName");

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

void packetDecodeContent(PContent *content, uint16_t len) {
    ZF_LOGD("Decode Content from Packet");
    assert(content->payload[0] == TT_Data);

    uint8_t type = 0;
    uint64_t length = 0;
    uint16_t offset = 0;

    for (; offset < len;) {
        type = content->payload[offset++];
        assert(type == TT_Data || type == TT_Name || type == TT_MetaInfo ||
               type == TT_Content);

        length = getGenericNameComponentLength(content->payload, &offset);

        if (type == TT_Content)
            break; // Found Content in packet
        if (type != TT_Data) {
            offset += length; // Skip this type's length until Content
        }
    }

    content->type = type;
    content->length = length;
    content->offset = offset;
}

void copyFromC(uint8_t *dst, uint16_t dst_off, uint8_t *src, uint16_t src_off,
               uint64_t count) {
    memcpy(&dst[dst_off], &src[src_off], count);
}
