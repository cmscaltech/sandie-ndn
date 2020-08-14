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
    ++(ct->cnt.nInterest);
}

static void ConsumerTx_EncodeReadInterest(ConsumerTx *ct, struct rte_mbuf *pkt,
                                          ConsumerTxRequest *req,
                                          uint64_t segNum) {
    uint8_t offsetNumComp[10];
    offsetNumComp[0] = TtByteOffsetNameComponent;
    offsetNumComp[1] = Nni_Encode(RTE_PTR_ADD(offsetNumComp, 2), segNum);

    uint16_t suffixL = req->nameL + offsetNumComp[1] + 2;
    uint8_t *suffixV = (uint8_t *)rte_malloc(NULL, suffixL, 0);
    rte_memcpy(suffixV, req->nameV, req->nameL);
    rte_memcpy(suffixV + req->nameL, &offsetNumComp, offsetNumComp[1] + 2);

    LName suffix = {.length = suffixL, .value = suffixV};
    uint32_t nonce = NonceGen_Next(&ct->nonceGen);
    InterestTemplate_Encode(&ct->readPrefixTpl, pkt, suffix, nonce);
}

static void ConsumerTx_ExpressReadInterests(ConsumerTx *ct,
                                            ConsumerTxRequest *req) {
    uint16_t nTx = req->npkts > CONSUMER_TX_MAX_BURST_SIZE
                       ? CONSUMER_TX_MAX_BURST_SIZE
                       : req->npkts;
    req->npkts -= nTx;

    ZF_LOGD("Sending %d READ Interest packet/s starting @%" PRIu64, nTx,
            req->off);

    struct rte_mbuf *pkts[nTx];
    int ret =
        rte_pktmbuf_alloc_bulk(ct->interestMp, (struct rte_mbuf **)pkts, nTx);

    if (unlikely(ret != 0)) {
        ZF_LOGW("interestMp-full");
        ct->onError(XRDNDNDPDK_EFAILURE);
        return;
    }

    for (uint16_t i = 0; i < nTx; ++i) {
        ConsumerTx_EncodeReadInterest(
            ct, pkts[i], req, req->off + i * XRDNDNDPDK_MAX_PAYLOAD_SIZE);
    }

    Face_TxBurst(ct->face, (Packet **)pkts, nTx);
    req->off += XRDNDNDPDK_MAX_PAYLOAD_SIZE * nTx;

    if (req->npkts > 0)
        rte_ring_enqueue(ct->requestQueue, req);
    ct->cnt.nInterest += nTx;
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
        if (unlikely(rte_ring_dequeue_burst(ct->requestQueue, &request, 1, 0) <=
                     0)) {
            rte_pause();
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
