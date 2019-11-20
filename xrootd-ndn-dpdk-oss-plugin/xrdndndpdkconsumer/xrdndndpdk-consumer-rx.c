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

#include "../../../ndn-dpdk/ndn/packet.h"

#include "xrdndndpdk-consumer-rx.h"

INIT_ZF_LOG(Xrdndndpdkconsumer);

// TODO:
static void ConsumerRx_processNackPacket(ConsumerRx *cr, Packet *npkt) {
    ZF_LOGD("Process Nack packet");
    ++cr->nNacks;

    // TODO: Check Nack reason and take actions accordingly
    // For the moment we treat Nack packet as error, so the consumer app will
    // stop
    cr->onError(XRDNDNDPDK_EFAILURE);
}

// Temporary solution for application level Nack
static void ConsumerRx_processMaxPacket(ConsumerRx *cr, Packet *npkt) {
    ZF_LOGD("Process MAX packet");
    ++cr->nErrors;
    // TODO: Process content and get nonNegativeInteger from it
    // After fstat impl
    cr->onError(XRDNDNDPDK_EFAILURE);
}

/**
 * @brief Read non-negative integer from Data packet
 *
 * @param content PContent structure
 * @return uint64_t Non-negative integer from Data packet
 */
static uint64_t ConsumerRx_readNonNegativeInteger(PContent *content) {
    ZF_LOGD("Read nonNegativeInteger from Data packet");

    rte_be64_t v;
    rte_memcpy(&v, &content->payload[content->offset], content->length);
    return rte_be_to_cpu_64(v);
}

/**
 * @brief Parse payload from Packet and return it back to consumer application
 *
 * @param cr Consumer Rx struct pointer
 * @param npkt Packet
 * @param content PContent struct
 */
static void ConsumerRx_processContent(ConsumerRx *cr, Packet *npkt,
                                      PContent *content) {
    ZF_LOGD("Process Content from Data packet");

    const LName name = *(const LName *)&Packet_GetDataHdr(npkt)->name;

    // TODO: Check content type Nack and go to nack callback
    // Need to implement encode and decode for MetaInfo ContentType

    SystemCallId sid = lnameGetSystemCallId(name);

    if (SYSCALL_OPEN_ID == sid) {
        ZF_LOGI("Return content for open filesystem call");

        uint64_t openRetCode = ConsumerRx_readNonNegativeInteger(content);
        rte_free(content->payload);
        rte_free(content);
        cr->onNonNegativeInteger(openRetCode);
    } else if (SYSCALL_FSTAT_ID == sid) {
        ZF_LOGI("Return content for stat filesystem call");

        cr->onContent(content, 0);
    } else if (likely(SYSCALL_READ_ID == sid)) {
        ZF_LOGI("Return content for read filesystem call @%d",
                lnameGetSegmentNumber(name));

        rte_free(content->payload);
        rte_free(content);
        //cr->onContent(NULL, lnameGetSegmentNumber(name));
    }
}

/**
 * @brief Process Data packet
 *
 * @param cr ConsumerRx struct pointer
 * @param npkt Packet received over NDN network
 */
static void ConsumerRx_processDataPacket(ConsumerRx *cr, Packet *npkt) {
    ZF_LOGD("Process new Data packet");

    NdnError e = Packet_ParseL3(npkt, NULL);
    if (unlikely(e != NdnError_OK)) {
        ZF_LOGF("Could not parseL3 Data");
        ++cr->nErrors;
        cr->onError(XRDNDNDPDK_EFAILURE);
        return;
    }

    struct rte_mbuf *pkt = Packet_ToMbuf(npkt);
    if (pkt->pkt_len == 0) {
        ZF_LOGE("Received packet with no payload");
        ++cr->nErrors;
        cr->onError(XRDNDNDPDK_EFAILURE);
        return;
    }

    int32_t length = pkt->pkt_len;
    // Higher level application needs to free the memory
    PContent *content = rte_malloc(NULL, sizeof(PContent), 0);
    content->payload = rte_malloc(NULL, length, 0);

    ZF_LOGD("Received packet with length: %d", length);
    cr->nBytes += length;

    // Composing payload from all nb_segs
    for (int offset = 0; NULL != pkt;) {
        rte_memcpy(&content->payload[offset], rte_pktmbuf_mtod(pkt, uint8_t *),
                   pkt->data_len);
        offset += pkt->data_len;
        pkt = pkt->next;
    }

    packetGetContent(content, length);
    ConsumerRx_processContent(cr, npkt, content);

    ++cr->nData;
}

/**
 * @brief Reset ConsumerTx struct counters used for statistic
 *
 * @param cr ConsumerRx struct pointer
 */
void ConsumerRx_resetCounters(ConsumerRx *cr) {
    cr->nData = 0;
    cr->nNacks = 0;
    cr->nBytes = 0;
    cr->nErrors = 0;
}

/**
 * @brief Main processing loop of current consumer thread
 *
 * @param cr ConsumerRx struct pointer
 */
void ConsumerRx_Run(ConsumerRx *cr) {
    ZF_LOGI("Started consumer Rx instance on socket: %d lcore %d",
            rte_socket_id(), rte_lcore_id());

    Packet *npkts[CONSUMER_MAX_BURST_SIZE];

    while (ThreadStopFlag_ShouldContinue(&cr->stop)) {
        uint16_t nRx = rte_ring_sc_dequeue_burst(cr->rxQueue, (void **)npkts,
                                                 CONSUMER_MAX_BURST_SIZE, NULL);
        for (uint16_t i = 0; i < nRx; ++i) {
            Packet *npkt = npkts[i];
            if (unlikely(Packet_GetL2PktType(npkt) != L2PktType_NdnlpV2)) {
                continue;
            }
            switch (Packet_GetL3PktType(npkt)) {
            case L3PktType_Data:
                ConsumerRx_processDataPacket(cr, npkt);
                break;
            case L3PktType_Nack:
                ConsumerRx_processNackPacket(cr, npkt);
                break;
            case L3PktType_MAX:
                ConsumerRx_processMaxPacket(cr, npkt);
                break;
            default:
                assert(false);
                break;
            }
        }
        FreeMbufs((struct rte_mbuf **)npkts, nRx);
    }
}
