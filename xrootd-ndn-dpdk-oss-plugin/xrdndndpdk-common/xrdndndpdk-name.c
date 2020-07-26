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

#include "xrdndndpdk-name.h"
#include "xrdndndpdk-tlv.h"
#include "xrdndndpdk-utils.h"

INIT_ZF_LOG(Xrdndndpdkcommon);

uint16_t Name_Decode_FilePath(const LName name, uint16_t off, char *filepath) {
    ZF_LOGD("Decode filepath from LName");
    assert(name.value[off] == TtGenericNameComponent);

    while (name.value[off] == TtGenericNameComponent && off < name.length) {
        ++off; // skip TLV-Type
        uint16_t length = 0;
        off +=
            TlvDecoder_ReadLength(&name.value[off], &length); // get TLV-Length
        strcat(filepath, "/");
        strncat(filepath, &name.value[off], length); // store TLV-Value
        off += length;
    }
    return off;
}

uint16_t Name_Decode_FilePathLength(const LName name, uint16_t off) {
    ZF_LOGD("Decode encoded filepath length");
    assert(name.value[off] == TtGenericNameComponent);

    while (name.value[off] == TtGenericNameComponent && off < name.length) {
        ++off;
        uint16_t length = 0;
        off += TlvDecoder_ReadLength(&name.value[off], &length);
        off += length;
    }
    return off;
}

PacketType Name_Decode_PacketType(const LName name) {
    ZF_LOGD("Decode packet type from LName");

    if (likely(PACKET_NAME_PREFIX_URI_READ_ENCODED_LEN <= name.length &&
               memcmp(PACKET_NAME_PREFIX_URI_READ_ENCODED, name.value,
                      PACKET_NAME_PREFIX_URI_READ_ENCODED_LEN) == 0)) {
        return PACKET_READ;
    } else if (PACKET_NAME_PREFIX_URI_FILEINFO_ENCODED_LEN <= name.length &&
               memcmp(PACKET_NAME_PREFIX_URI_FILEINFO_ENCODED, name.value,
                      PACKET_NAME_PREFIX_URI_FILEINFO_ENCODED_LEN) == 0) {
        return PACKET_FILEINFO;
    }

    return PACKET_NOT_SUPPORTED;
}
