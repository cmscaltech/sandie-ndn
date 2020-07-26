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

INIT_ZF_LOG(Xrdndndpdkcommon);

/**
 * @brief Parse TLV-LENGTH number
 *
 * @param room Buffer to start of TLV-LENGTH
 * @param n The number
 * @return uint16_t The size of n
 */
uint16_t TlvDecoder_ReadLength(const uint8_t *room, uint16_t *n) {
    ZF_LOGV("Read TLV-Length number");

    if (room[0] <= 0xFC) {
        *n = *room;
        return 1;
    } else if (likely(room[0] == 0xFD)) {
        *n = rte_be_to_cpu_16(*(unaligned_uint16_t *)(room + 1));
        return 3;
    }

    assert(room[0] <= 0xFC || room[0] == 0xFD);
    return 1;
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
    if (likely(n < 0xFD)) { // 253
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

uint8_t *TlvEncoder_Append(struct rte_mbuf *m, uint16_t len) {
    assert(len <= rte_pktmbuf_tailroom(m));
    uint16_t off = m->data_len;
    m->pkt_len = m->data_len = off + len;
    return rte_pktmbuf_mtod_offset(m, uint8_t *, off);
}

void TlvEncoder_AppendVarNum(struct rte_mbuf *m, uint32_t n) {
    uint8_t *room = TlvEncoder_Append(m, TlvEncoder_SizeofVarNum(n));
    assert(room != NULL);
    TlvEncoder_WriteVarNum(room, n);
}

void TlvEncoder_PrependTL(struct rte_mbuf *m, uint32_t type, uint32_t length) {
    uint16_t sizeT = TlvEncoder_SizeofVarNum(type);
    uint16_t sizeL = TlvEncoder_SizeofVarNum(length);
    uint8_t *room = (uint8_t *)rte_pktmbuf_prepend(m, sizeT + sizeL);
    TlvEncoder_WriteVarNum(room, type);
    TlvEncoder_WriteVarNum((uint8_t *)RTE_PTR_ADD(room, sizeT), length);
}
