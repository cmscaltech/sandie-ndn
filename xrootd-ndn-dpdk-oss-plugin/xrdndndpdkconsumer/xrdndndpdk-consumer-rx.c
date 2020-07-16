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

#include "ndn-dpdk/ndn/packet.h"

#include "xrdndndpdk-consumer-rx.h"

INIT_ZF_LOG(Xrdndndpdkconsumer);

static void ConsumerRx_processNackPacket(ConsumerRx *cr, Packet *npkt) {
    ZF_LOGD("Process Nack packet");
    ++cr->nNacks;

    cr->onError(XRDNDNDPDK_EFAILURE);
}

// Temporary solution for Application Level Nack
static void ConsumerRx_processMaxPacket(ConsumerRx *cr, Packet *npkt) {
    ZF_LOGD("Process MAX packet");
    ++cr->nErrors;

    // TODO: Process content and get nonNegativeInteger from it
    cr->onError(XRDNDNDPDK_EFAILURE);
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

    const LName *name = (const LName *)&Packet_GetDataHdr(npkt)->name;

    // TODO: Check content type Nack and go to nack callback
    // Need to implement encode and decode for MetaInfo ContentType

    PacketType pt = lnameGetPacketType(*name);

    if (likely(PACKET_READ == pt)) {
        uint16_t len = lnameGetFilePathLength(
            *name, PACKET_NAME_PREFIX_URI_READ_ENCODED_LEN);
        uint64_t segNum = 0;
        NdnError e =
            DecodeNni(name->value[len + 1], &(name->value[len + 2]), &segNum);
        if (unlikely(e != NdnError_OK)) {
            ZF_LOGF("Could not decode Non-Negative Integer");
            cr->onError(XRDNDNDPDK_EFAILURE);
        }

        cr->onContent(content, segNum);
    } else if (PACKET_FILEINFO == pt) {
        ZF_LOGD("Return FILEINFO content of size: %" PRIu64, content->length);

        if (content->length <= sizeof(uint64_t)) {
            cr->onError(XRDNDNDPDK_EFAILURE);
        } else {
            cr->onContent(content, 0);
        }
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

    packetDecodeContent(content, length);
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
        uint32_t nRx =
            PktQueue_Pop(&cr->rxQueue, (struct rte_mbuf **)npkts,
                         CONSUMER_MAX_BURST_SIZE, rte_get_tsc_cycles())
                .count;

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
