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

INIT_ZF_LOG(Xrdndndpdkconsumer);

void ConsumerTx_resetCounters(ConsumerTx *ct) { ct->nInterests = 0; }

static void ConsumerTx_ExpressFileInfoInterest(ConsumerTx *ct,
                                               ConsumerTxRequest *req) {
    ZF_LOGD("Send Interest packet for FILEINFO");

    if (unlikely(req->nameL + 10 +
                     RTE_MAX(PACKET_NAME_PREFIX_URI_READ_ENCODED_LEN,
                             PACKET_NAME_PREFIX_URI_FILEINFO_ENCODED_LEN) >
                 XRDNDNDPDK_MAX_NAME_SIZE)) {
        ZF_LOGW("The Interest Name is too long to fit in packet");
        ct->onError(XRDNDNDPDK_EFAILURE);
        return;
    }

    struct rte_mbuf *pkt = rte_pktmbuf_alloc(ct->interestMp);
    if (NULL == pkt) {
        ZF_LOGF("interestMp-full");
        ct->onError(XRDNDNDPDK_EFAILURE);
        return;
    }

    LName suffix = {.length = req->nameL, .value = req->nameV};
    uint32_t nonce = NonceGen_Next(&ct->nonceGen);

    InterestTemplate_Encode(&ct->fileInfoPrefixTpl, pkt, suffix, nonce);
    Face_Tx(ct->face, Packet_FromMbuf(pkt));
    ++ct->nInterests;
}

static void ConsumerTx_EncodeReadInterest(ConsumerTx *ct, struct rte_mbuf *pkt,
                                          ConsumerTxRequest *req,
                                          uint64_t segNum) {
    uint8_t segNumComp[10];
    segNumComp[0] = TtSegmentNameComponent;
    segNumComp[1] = Nni_Encode(RTE_PTR_ADD(segNumComp, 2), segNum);

    uint16_t suffixL = req->nameL + segNumComp[1] + 2;
    uint8_t *suffixV = (uint8_t *)rte_malloc(NULL, suffixL, 0);
    rte_memcpy(suffixV, req->nameV, req->nameL);
    rte_memcpy(suffixV + req->nameL, &segNumComp, segNumComp[1] + 2);

    LName suffix = {.length = suffixL, .value = suffixV};
    uint32_t nonce = NonceGen_Next(&ct->nonceGen);
    InterestTemplate_Encode(&ct->readPrefixTpl, pkt, suffix, nonce);
}

static void ConsumerTx_ExpressReadInterests(ConsumerTx *ct,
                                            ConsumerTxRequest *req) {
    ZF_LOGD("Sending %d READ Interest packet/s starting @%" PRIu64, req->npkts,
            req->off);

    struct rte_mbuf *pkts[req->npkts];
    int ret = rte_pktmbuf_alloc_bulk(ct->interestMp, (struct rte_mbuf **)pkts,
                                     req->npkts);

    if (unlikely(ret != 0)) {
        ZF_LOGW("interestMp-full");
        ct->onError(XRDNDNDPDK_EFAILURE);
        return;
    }

    for (uint16_t i = 0; i < req->npkts; ++i) {
        ConsumerTx_EncodeReadInterest(ct, pkts[i], req,
                                      req->off + i * XRDNDNDPDK_PACKET_SIZE);
    }

    Face_TxBurst(ct->face, (Packet **)pkts, req->npkts);
    ct->nInterests += req->npkts;
}

typedef void (*OnRequest)(ConsumerTx *ct, ConsumerTxRequest *req);

static const OnRequest onRequest[PACKET_MAX] = {
    [PACKET_FILEINFO] = ConsumerTx_ExpressFileInfoInterest,
    [PACKET_READ] = ConsumerTx_ExpressReadInterests,
};

int ConsumerTx_Run(ConsumerTx *ct) {
    ZF_LOGI("Started consumer Tx instance on socket: %d lcore %d",
            rte_socket_id(), rte_lcore_id());

    while (ThreadStopFlag_ShouldContinue(&ct->stop)) {
        void *request;
        if (rte_ring_dequeue_burst(ct->requestQueue, &request, 1, 0) <= 0) {
            // rte_pause();
            continue;
        }

        if (unlikely(((ConsumerTxRequest *)request)->pt >= PACKET_MAX)) {
            ZF_LOGW("Unsupported packet type");
            ct->onError(XRDNDNDPDK_EFAILURE);
            return XRDNDNDPDK_EFAILURE;
        }

        onRequest[((ConsumerTxRequest *)request)->pt](
            ct, (ConsumerTxRequest *)request);
    }

    return XRDNDNDPDK_ESUCCESS;
}
