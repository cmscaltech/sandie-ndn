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

    interestPkt->data_off = ct->interestMbufHeadroom;
    EncodeInterest(interestPkt, &ct->prefixTpl, ct->prefixTplBuf, *suffix,
                   NonceGen_Next(&ct->nonceGen), 0, NULL);

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
    ZF_LOGD(
        "Sending %d Interest packets for read file system call starting @%d", n,
        off);

    assert(n <= CONSUMER_MAX_BURST_SIZE);

    Packet *npkts[n];
    int ret =
        rte_pktmbuf_alloc_bulk(ct->interestMp, (struct rte_mbuf **)npkts, n);

    if (unlikely(ret != 0)) {
        ZF_LOGW("interestMp-full");
        return;
    }

    ZF_LOGD("Have to copy: %d packets", n);
    for (uint16_t step = 0; step < n; step += 16) {
        ZF_LOGD ("Copy from: %d to %d", step, step+16);
        for (uint16_t i = step; i < step + 16 && i < n; ++i) {
            struct rte_mbuf *pkt = Packet_ToMbuf(npkts[i]);
            pkt->data_off = ct->interestMbufHeadroom;

            ct->segmentNumberComponent.compV = off + i * XRDNDNDPDK_PACKET_SIZE;

            rte_memcpy(ct->suffixBuffer, path->value, path->length);
            rte_memcpy(ct->suffixBuffer + path->length,
                    &ct->segmentNumberComponent.compT,
                    SEGMENT_NO_COMPONENT_SIZE);

            LName suffix = {.length = path->length + SEGMENT_NO_COMPONENT_SIZE,
                            .value = ct->suffixBuffer};

            EncodeInterest(pkt, &ct->readPrefixTpl, ct->readPrefixTplBuf, suffix,
                        NonceGen_Next(&ct->nonceGen), 0, NULL);
            Packet_SetL3PktType(npkts[i], L3PktType_Interest);
            Packet_InitLpL3Hdr(npkts[i]);
        }

        // ZF_LOGD("sleep 1ms");
        rte_delay_us(50);
    }

    Face_TxBurst(ct->face, npkts, n);
    ct->nInterests += n;
}
