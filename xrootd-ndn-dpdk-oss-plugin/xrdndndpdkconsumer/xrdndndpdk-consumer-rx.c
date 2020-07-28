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

#include <sys/stat.h>

#include "ndn-dpdk/csrc/ndni/packet.h"

#include "xrdndndpdk-consumer-rx.h"

INIT_ZF_LOG(Xrdndndpdkconsumer);

__attribute__((nonnull)) static void
ConsumerRx_processContent(ConsumerRx *cr, Packet *npkt, PContent *content) {
    ZF_LOGD("Process Content from Data packet");

    if (unlikely(Data_IsAppLvlNack(content))) {
        uint64_t err = 0;
        Nni_Decode(content->contentL, content->contentV, &err);

        ZF_LOGW("Received Application-Level Nack with content: %" PRIu64, err);
        rte_free(content->payload);
        rte_free(content);

        cr->onError(err);
        return;
    }

    const LName *name = (const LName *)&Packet_GetDataHdr(npkt)->name;
    PacketType pt = Name_Decode_PacketType(*name);

    if (likely(PACKET_READ == pt)) {
        const uint8_t *segNumComp = RTE_PTR_ADD(
            name->value, Name_Decode_FilePathLength(
                             *name, PACKET_NAME_PREFIX_URI_READ_ENCODED_LEN));
        assert(segNumComp[0] == TtSegmentNameComponent);

        uint64_t segNum = 0;
        Nni_Decode(segNumComp[1], RTE_PTR_ADD(segNumComp, 2), &segNum);

        cr->onContent(content->contentL, segNum);
    } else if (PACKET_FILEINFO == pt) {
        ZF_LOGD("Return FILEINFO content of size: %" PRIu16, content->contentL);
        cr->onContent(((struct stat *)content->contentV)->st_size, 0);
    }

    // Do not send payload back to Golang - performance issue
    rte_free(content->payload);
    rte_free(content);
}

__attribute__((nonnull)) static void
ConsumerRx_processNackPacket(ConsumerRx *cr, Packet *npkt) {
    ZF_LOGD("Process new Nack packet");
    cr->cnt.nNack++;
    cr->onError(XRDNDNDPDK_EFAILURE);
}

__attribute__((nonnull)) static void
ConsumerRx_processDataPacket(ConsumerRx *cr, Packet *npkt) {
    ZF_LOGD("Process new Data packet");
    struct rte_mbuf *pkt = Packet_ToMbuf(npkt);

    PContent *content = rte_malloc(NULL, sizeof(PContent), 0);
    content->payload = rte_malloc(NULL, pkt->pkt_len, 0);
    Mbuf_CopyTo(pkt, content->payload);

    // Update counters
    cr->cnt.nBytes += pkt->pkt_len;
    ++(cr->cnt.nData);

    Data_Decode(content, pkt->pkt_len);
    ConsumerRx_processContent(cr, npkt, content);
}

/**
 * @brief Main processing loop of current consumer thread
 *
 * @param cr ConsumerRx struct pointer
 */
int ConsumerRx_Run(ConsumerRx *cr) {
    ZF_LOGI("Started consumer Rx instance on socket: %d lcore %d",
            rte_socket_id(), rte_lcore_id());

    struct rte_mbuf *pkts[CONSUMER_RX_MAX_BURST_SIZE];

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
