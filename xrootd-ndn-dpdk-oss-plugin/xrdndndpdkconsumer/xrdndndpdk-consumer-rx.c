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


#include <stdio.h>

#include "../../../ndn-dpdk/core/logger.h"
#include "../../../ndn-dpdk/ndn/packet.h"

#include "xrdndndpdk-consumer-rx.h"

INIT_ZF_LOG(Xrdndndpdkconsumer);

// TODO
static void ConsumerRx_ProcessNack(ConsumerRx *cr, Packet *npkt) {
    ZF_LOGD("Process Nack packet");
    ++cr->nNacks;
}

/**
 * @brief Read non-negative integer from Data packet
 *
 * @param content PContent structure
 * @return uint64_t Non-negative integer from Data packet
 */
static uint64_t ConsumerRx_readNonNegativeInteger_Data(PContent *content) {
    ZF_LOGD("Reading non-negative integer from Data packet");

    rte_be64_t v;
    rte_memcpy(&v, content->buff, content->length);
    return rte_be_to_cpu_64(v);
}

/**
 * @brief Parse payload from Packet and return it back to consumer application
 * 
 * @param cr Consumer Rx struct pointer
 * @param npkt Packet
 * @param content PContent struct
 */
static void ConsumerRx_ProcessContent(ConsumerRx *cr, Packet *npkt,
                                  PContent *content) {
    ZF_LOGD("Return Data to consumer application");

    const LName name = *(const LName *)&Packet_GetDataHdr(npkt)->name;

    // TODO: Check content type Nack and go to nack callback

    SystemCallId sid = lnameGetSystemCallId(name);
    if (SYSCALL_OPEN_ID == sid) {
      ZF_LOGD("Return content for file open systemcall");
      uint64_t openRetCode = ConsumerRx_readNonNegativeInteger_Data(content);
      cr->onOpenData(openRetCode);
    } else if (SYSCALL_FSTAT_ID == sid) {
      ZF_LOGD("Return content for file stat systemcall");
      // TODO
    } else if (likely(SYSCALL_READ_ID == sid)) {
      ZF_LOGD("Return content for file read systemcall");
      // TODO
    }
}

static void ConsumerRx_ProcessPacket(ConsumerRx *cr, Packet *npkt) {
    ZF_LOGD("Consumer Rx thread processing Data packet");

    NdnError e = Packet_ParseL3(npkt, NULL);
    if (unlikely(e != NdnError_OK)) {
        ZF_LOGF("Could not parseL3 Data");
        cr->onError();
        return;
    }

    struct rte_mbuf *pkt = Packet_ToMbuf(npkt);
    if (pkt->pkt_len == 0) {
        ZF_LOGE("Received packet with no payload");
        cr->onError();
        return;
    }

    int32_t length = pkt->pkt_len;
    uint8_t *payload = rte_malloc(NULL, length, 0);

    // Composing payload from all nb_segs
    for (int offset = 0; NULL != pkt;) {
        rte_memcpy(&payload[offset], rte_pktmbuf_mtod(pkt, uint8_t *),
                   pkt->data_len);
        offset += pkt->data_len;
        pkt = pkt->next;
    }

    PContent content = packetGetContent(payload, length);
    content.buff = rte_malloc(NULL, content.length, 0);
    // Store entire Content in PContent
    rte_memcpy(content.buff, &payload[content.offset], content.length);
    rte_free(payload);

    ConsumerRx_ProcessContent(cr, npkt, &content);

    rte_free(content.buff);
    ++cr->nData;
}

void ConsumerRx_Run(ConsumerRx *cr) {
    ZF_LOGD("Started Consumer Rx thread");

    Packet *npkts[CONSUMER_RX_BURST_SIZE];

    while (ThreadStopFlag_ShouldContinue(&cr->stop)) {
        uint16_t nRx = rte_ring_sc_dequeue_burst(cr->rxQueue, (void **)npkts,
                                                 CONSUMER_RX_BURST_SIZE, NULL);
        for (uint16_t i = 0; i < nRx; ++i) {
            Packet *npkt = npkts[i];
            if (unlikely(Packet_GetL2PktType(npkt) != L2PktType_NdnlpV2)) {
                continue;
            }
            switch (Packet_GetL3PktType(npkt)) {
            case L3PktType_Data:
                ConsumerRx_ProcessPacket(cr, npkt);
                break;
            case L3PktType_Nack:
                ConsumerRx_ProcessNack(cr, npkt);
                break;
            default:
                assert(false);
                break;
            }
        }
        FreeMbufs((struct rte_mbuf **)npkts, nRx);
    }
}
