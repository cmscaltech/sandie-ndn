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

#include "xrdndndpdk-tlv.h"

INIT_ZF_LOG(Xrdndndpdkutils);

uint64_t TlvDecoder_GenericNameComponentLength(const uint8_t *buf,
                                               uint16_t *off) {
    ZF_LOGV("Decode Name component length");
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

static __rte_always_inline uint8_t *TlvEncoder_EncodeVarNum(uint8_t *room,
                                                            uint32_t n) {
    if (likely(n < 253)) {
        room[0] = (uint8_t)n;
        return room + 1;
    }

    if (likely(n <= UINT16_MAX)) {
        room[0] = 253;
        room[1] = (uint8_t)(n >> 8);
        room[2] = (uint8_t)n;
        return room + 3;
    }

    *room++ = 254;
    rte_be32_t v = rte_cpu_to_be_32((uint32_t)n);
    rte_memcpy(room, &v, 4);
    return room + 4;
}

/**
 * @brief Compute size of VAR-NUMBER
 *
 * @param n Number
 * @return __rte_always_inline Size
 */
static __rte_always_inline uint8_t TlvEncoder_SizeofVarNum(uint32_t n) {
    if (likely(n < 0xFD)) {
        return 1;
    } else if (likely(n <= UINT16_MAX)) {
        return 3;
    } else {
        return 5;
    }
}

/**
 * @brief Write VAR-NUMBER to the given buffer.
 * @param[out] room output buffer, must have sufficient size.
 */
__attribute__((nonnull)) static __rte_always_inline void
TlvEncoder_WriteVarNum(uint8_t *room, uint32_t n) {
    assert(room != NULL);
    if (likely(n < 0xFD)) {
        room[0] = n;
    } else if (likely(n <= UINT16_MAX)) {
        room[0] = 0xFD;
        unaligned_uint16_t *b = RTE_PTR_ADD(room, 1);
        *b = rte_cpu_to_be_16(n);
    } else {
        room[0] = 0xFE;
        unaligned_uint32_t *b = RTE_PTR_ADD(room, 1);
        *b = rte_cpu_to_be_32(n);
    }
}

uint8_t *TlvEncoder_AppendTLV(struct rte_mbuf *m, uint16_t len) {
    assert(len <= rte_pktmbuf_tailroom(m));
    uint16_t off = m->data_len;
    m->pkt_len = m->data_len = off + len;
    return rte_pktmbuf_mtod_offset(m, uint8_t *, off);
}

void TlvEncoder_AppendTL(struct rte_mbuf *m, uint32_t n) {
    uint8_t *room = TlvEncoder_AppendTLV(m, TlvEncoder_SizeofVarNum(n));
    assert(room != NULL);
    TlvEncoder_EncodeVarNum(room, n);
}

void TlvEncoder_PrependTL(struct rte_mbuf *m, uint32_t type, uint32_t length) {
    uint16_t sizeT = TlvEncoder_SizeofVarNum(type);
    uint16_t sizeL = TlvEncoder_SizeofVarNum(length);
    uint8_t *room = (uint8_t *)rte_pktmbuf_prepend(m, sizeT + sizeL);
    TlvEncoder_WriteVarNum(room, type);
    TlvEncoder_WriteVarNum((uint8_t *)RTE_PTR_ADD(room, sizeT), length);
}
