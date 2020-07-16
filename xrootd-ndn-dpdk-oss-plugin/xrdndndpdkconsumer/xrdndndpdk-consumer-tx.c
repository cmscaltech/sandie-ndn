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

#include "xrdndndpdk-consumer-tx.h"
#include "ndn-dpdk/ndn/nni.h"

INIT_ZF_LOG(Xrdndndpdkconsumer);

/**
 * @brief Reset ConsumerTx struct counters used for statistic
 *
 * @param ct ConsumerTx struct pointer
 */
void ConsumerTx_resetCounters(ConsumerTx *ct) { ct->nInterests = 0; }

void ConsumerTx_ExpressFileInfoInterest(ConsumerTx *ct, struct LName *path) {
    ZF_LOGD("Send Interest packet for FILEINFO");
    struct rte_mbuf *pkt = rte_pktmbuf_alloc(ct->interestMp);
    if (NULL == pkt) {
        ZF_LOGF("interestMp-full");
        ct->onError(XRDNDNDPDK_EFAILURE);
        return;
    }

    EncodeInterest(pkt, &ct->fileInfoPrefixTpl, *path,
                   NonceGen_Next(&ct->nonceGen));
    Packet_SetL3PktType(Packet_FromMbuf(pkt), L3PktType_Interest);
    Packet_InitLpL3Hdr(Packet_FromMbuf(pkt));
    Face_Tx(ct->face, Packet_FromMbuf(pkt));
    ++ct->nInterests;
}

static void ConsumerTx_EncodeReadInterest(ConsumerTx *ct, Packet *npkt,
                                          struct LName *path, uint64_t segNum) {
    uint8_t segNumber[10];
    segNumber[0] = TT_SegmentNameComponent;
    segNumber[1] = EncodeNni(&segNumber[2], segNum);

    uint8_t suffixBuffer[NAME_MAX_LENGTH];
    rte_memcpy(suffixBuffer, path->value, path->length);
    rte_memcpy(suffixBuffer + path->length, &segNumber, 10);

    LName suffix = {.length = path->length + segNumber[1] + 2,
                    .value = suffixBuffer};

    struct rte_mbuf *pkt = Packet_ToMbuf(npkt);
    EncodeInterest(pkt, &ct->readPrefixTpl, suffix,
                   NonceGen_Next(&ct->nonceGen));

    Packet_SetL3PktType(npkt, L3PktType_Interest);
    Packet_InitLpL3Hdr(npkt);
}

void ConsumerTx_ExpressReadInterests(ConsumerTx *ct, struct LName *path,
                                     uint64_t off, uint16_t n) {
    ZF_LOGD("Sending %d READ Interest packetsstarting @%" PRIu64, n, off);

    Packet *npkts[n];
    int ret =
        rte_pktmbuf_alloc_bulk(ct->interestMp, (struct rte_mbuf **)npkts, n);

    if (unlikely(ret != 0)) {
        ZF_LOGW("interestMp-full");
        return;
    }

    for (uint16_t i = 0; i < n; ++i) {
        ConsumerTx_EncodeReadInterest(ct, npkts[i], path,
                                      off + i * XRDNDNDPDK_PACKET_SIZE);
    }

    Face_TxBurst(ct->face, npkts, n);
    ct->nInterests += n;
}
