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

#include "xrdndndpdk-data.h"
#include "xrdndndpdk-tlv.h"

INIT_ZF_LOG(Xrdndndpdkcommon);

// clang-format off
static const uint8_t FAKESIG[] = {
    TtDSigInfo, 0x03,
    TtSigType, 0x01, SigHmacWithSha256,
    TtDSigValue, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
// clang-format on

void *Data_Encode_(struct rte_mbuf *m, uint16_t nameL, const uint8_t *nameV,
                   uint8_t contentType, uint32_t freshnessPeriod,
                   uint16_t contentL, const uint8_t *contentV) {
    ZF_LOGD("Encode Data");

    { // Name
        assert(nameL > 0);
        TlvEncoder_AppendTL(m, TtName);
        TlvEncoder_AppendTL(m, nameL);
        rte_memcpy(rte_pktmbuf_append(m, nameL), nameV, nameL);
    }

    { // MetaInfo
        typedef struct MetaInfoF {
            uint8_t metaInfoT;
            uint8_t metaInfoL;
            uint8_t contentTypeT;
            uint8_t contentTypeL;
            uint8_t contentTypeV;
            uint8_t freshnessPeriodT;
            uint8_t freshnessPeriodL;
            rte_be32_t freshnessPeriodV;
        } __rte_packed MetaInfoF;

        MetaInfoF *f = (MetaInfoF *)TlvEncoder_AppendTLV(m, sizeof(MetaInfoF));
        f->metaInfoT = TtMetaInfo;
        f->metaInfoL = 9;

        // ContentType
        // https://named-data.net/doc/NDN-packet-spec/current/types.html
        f->contentTypeT = 0x18;
        f->contentTypeL = 1;
        f->contentTypeV = contentType;

        f->freshnessPeriodT = TtFreshnessPeriod;
        f->freshnessPeriodL = 4;
        *(unaligned_uint32_t *)&f->freshnessPeriodV =
            rte_cpu_to_be_32(freshnessPeriod);
    }

    { // Content
        if (contentL != 0) {
            TlvEncoder_AppendTL(m, TtContent);
            TlvEncoder_AppendTL(m, contentL);
            rte_memcpy(rte_pktmbuf_append(m, contentL), contentV, contentL);
        }
    }

    // Data Signature
    rte_memcpy(rte_pktmbuf_append(m, sizeof(FAKESIG)), FAKESIG,
               sizeof(FAKESIG));

    m->vlan_tci = nameL;
    TlvEncoder_PrependTL(m, TtData, m->pkt_len);

    return (void *)m;
}

void Data_Decode(PContent *content, uint16_t len) {
    ZF_LOGD("Decode Content from Packet");
    assert(content->payload[0] == TtData);

    uint8_t type = 0;
    uint64_t length = 0;
    uint16_t offset = 0;

    while (offset < len) {
        type = content->payload[offset++];
        assert(type == TtData || type == TtName || type == TtMetaInfo ||
               type == TtContent);

        length =
            TlvDecoder_GenericNameComponentLength(content->payload, &offset);

        if (type == TtMetaInfo) {
            assert(content->payload[offset] == 0x18);
            content->type = content->payload[offset + 2];
        }

        if (type == TtContent)
            break; // Found Content in packet
        if (type != TtData) {
            offset += length; // Skip this type's length until Content
        }
    }

    content->length = length;
    content->offset = offset;
}
