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

#include "ndn-dpdk/csrc/ndni/packet.h"

#include "xrdndndpdk-consumer-rx.h"

INIT_ZF_LOG(Xrdndndpdkconsumer);

static void ConsumerRx_processContent(ConsumerRx *cr, Packet *npkt,
                                      PContent *content) {
    ZF_LOGD("Process Content from Data packet");

    const LName *name = (const LName *)&Packet_GetDataHdr(npkt)->name;

    // TODO: Check content type Nack and go to nack callback
    // Need to implement encode and decode for MetaInfo ContentType

    PacketType pt = Name_Decode_PacketType(*name);

    if (likely(PACKET_READ == pt)) {
        const uint8_t *segNumComp = RTE_PTR_ADD(
            name->value, Name_Decode_FilePathLength(
                             *name, PACKET_NAME_PREFIX_URI_READ_ENCODED_LEN));
        assert(segNumComp[0] == TtSegmentNameComponent);

        uint64_t segNum = 0;
        Nni_Decode(segNumComp[1], RTE_PTR_ADD(segNumComp, 2), &segNum);

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

static void ConsumerRx_processNackPacket(ConsumerRx *cr, Packet *npkt) {
    ZF_LOGD("Process new Nack packet");
    ++cr->nNacks;

    cr->onError(XRDNDNDPDK_EFAILURE);
}

static void ConsumerRx_processDataPacket(ConsumerRx *cr, Packet *npkt) {
    ZF_LOGD("Process new Data packet");

    struct rte_mbuf *pkt = Packet_ToMbuf(npkt);
    int32_t length = pkt->pkt_len;
    if (pkt->pkt_len == 0) {
        ZF_LOGE("Received packet with no payload");
        ++cr->nErrors;
        cr->onError(XRDNDNDPDK_EFAILURE);
        return;
    }

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

    Data_Decode(content, length);
    ConsumerRx_processContent(cr, npkt, content);

    ++cr->nData;
}

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
int ConsumerRx_Run(ConsumerRx *cr) {
    ZF_LOGI("Started consumer Rx instance on socket: %d lcore %d",
            rte_socket_id(), rte_lcore_id());

    struct rte_mbuf *pkts[CONSUMER_MAX_BURST_SIZE];

    while (ThreadStopFlag_ShouldContinue(&cr->stop)) {
        uint32_t nRx = PktQueue_Pop(&cr->rxQueue, (struct rte_mbuf **)pkts,
                                    RTE_DIM(pkts), rte_get_tsc_cycles())
                           .count;

        for (uint16_t i = 0; i < nRx; ++i) {
            Packet *npkt = Packet_FromMbuf(pkts[i]);

            switch (Packet_GetType(npkt)) {
            case PktData:
                ConsumerRx_processDataPacket(cr, npkt);
                break;
            case PktNack:
                ConsumerRx_processNackPacket(cr, npkt);
                break;
            default:
                ZF_LOGW("Received unsupported packet type");
                assert(false);
                break;
            }
        }

        rte_pktmbuf_free_bulk_(pkts, nRx);
    }

    return 0;
}
