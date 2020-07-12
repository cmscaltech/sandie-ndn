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

#include "xrdndndpdk-consumer-tx.h"
#include "ndn-dpdk/ndn/nni.h"

INIT_ZF_LOG(Xrdndndpdkconsumer);

/**
 * @brief Reset ConsumerTx struct counters used for statistic
 *
 * @param ct ConsumerTx struct pointer
 */
void ConsumerTx_resetCounters(ConsumerTx *ct) { ct->nInterests = 0; }

/**
 * @brief Send one Interest packet over NDN network
 *
 * @param ct ConsumerTx struct pointer
 * @param suffix Suffix of Interest Name
 */
void ConsumerTx_sendInterest(ConsumerTx *ct, struct LName *suffix) {
    ZF_LOGD("Send Interest packet for open/fstat file system call");

    struct rte_mbuf *interestPkt = rte_pktmbuf_alloc(ct->interestMp);
    if (NULL == interestPkt) {
        ZF_LOGF("interestMp-full");
        ct->onError(XRDNDNDPDK_EFAILURE);
        return;
    }

    EncodeInterest(interestPkt, &ct->prefixTpl, *suffix,
                   NonceGen_Next(&ct->nonceGen));

    Packet_SetL3PktType(Packet_FromMbuf(interestPkt), L3PktType_Interest);
    Packet_InitLpL3Hdr(Packet_FromMbuf(interestPkt));
    Face_Tx(ct->face, Packet_FromMbuf(interestPkt));
    ++ct->nInterests;
}

/**
 * @brief Send multiple Interest packets over NDN network. Used for read file
 * system call
 *
 * @param ct ConsumerTx struct pointer
 * @param path File path
 * @param off Starting point of segment number composion
 * @param n Number of packets to send
 */
void ConsumerTx_sendInterests(ConsumerTx *ct, struct LName *path, uint64_t off,
                              uint16_t n) {
    ZF_LOGD("Sending %d Interest packets for read file system call starting "
            "@%" PRIu64,
            n, off);

    Packet *npkts[n];
    int ret =
        rte_pktmbuf_alloc_bulk(ct->interestMp, (struct rte_mbuf **)npkts, n);

    if (unlikely(ret != 0)) {
        ZF_LOGW("interestMp-full");
        return;
    }

    for (uint16_t i = 0; i < n; ++i) {
        struct rte_mbuf *pkt = Packet_ToMbuf(npkts[i]);

        uint8_t segNumber[10];
        segNumber[0] = TT_SegmentNameComponent;
        segNumber[1] =
            EncodeNni(&segNumber[2], off + i * XRDNDNDPDK_PACKET_SIZE);

        uint8_t suffixBuffer[NAME_MAX_LENGTH];
        rte_memcpy(suffixBuffer, path->value, path->length);
        rte_memcpy(suffixBuffer + path->length, &segNumber, 10);

        LName suffix = {.length = path->length + segNumber[1] + 2,
                        .value = suffixBuffer};

        EncodeInterest(pkt, &ct->readPrefixTpl, suffix,
                       NonceGen_Next(&ct->nonceGen));

        Packet_SetL3PktType(npkts[i], L3PktType_Interest);
        Packet_InitLpL3Hdr(npkts[i]);
    }

    Face_TxBurst(ct->face, npkts, n);
    ct->nInterests += n;
}
