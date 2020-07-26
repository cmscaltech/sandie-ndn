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

#include "../xrdndndpdk-common/xrdndndpdk-data.h"
#include "../xrdndndpdk-common/xrdndndpdk-name.h"
#include "xrdndndpdk-producer.h"

INIT_ZF_LOG(Xrdndndpdkproducer);

static Packet *Producer_EncodeData(Producer *producer, Packet *npkt,
                                   uint64_t length, uint8_t *content) {
    ZF_LOGD("Encode Data with content length: %" PRIu64, length);

    uint64_t token = Packet_GetLpL3Hdr(npkt)->pitToken;
    const LName *name = (const LName *)&Packet_GetInterestHdr(npkt)->name;

    struct rte_mbuf *pkt = rte_pktmbuf_alloc(producer->dataMp);
    if (unlikely(pkt == NULL)) {
        ZF_LOGW("dataMp-full");
        rte_pktmbuf_free(Packet_ToMbuf(npkt));
        return NULL;
    }

    Data_Encode(pkt, name->length, name->value, producer->freshnessPeriod,
                length, content);
    rte_pktmbuf_free(Packet_ToMbuf(npkt));

    Packet *response = Packet_FromMbuf(pkt);
    Packet_SetType(response, PktSData);
    *Packet_GetLpL3Hdr(response) = (const LpL3){0};
    Packet_GetLpL3Hdr(response)->pitToken = token;

    return response;
}

static Packet *Producer_EncodeDataAsError(Producer *producer, Packet *npkt,
                                          uint64_t content) {
    ZF_LOGI("Encode Application-Level Nack with content: %" PRIu64, content);

    uint64_t token = Packet_GetLpL3Hdr(npkt)->pitToken;
    const LName *name = (const LName *)&Packet_GetInterestHdr(npkt)->name;

    struct rte_mbuf *pkt = rte_pktmbuf_alloc(producer->dataMp);
    if (unlikely(pkt == NULL)) {
        ZF_LOGW("dataMp-full");
        rte_pktmbuf_free(Packet_ToMbuf(npkt));
        return NULL;
    }

    uint8_t err[8];
    int length = Nni_Encode(&err[0], content);
    Data_Encode_AppLvlNack(pkt, name->length, name->value, 0, length, &err[0]);

    rte_pktmbuf_free(Packet_ToMbuf(npkt));

    Packet *response = Packet_FromMbuf(pkt);
    Packet_SetType(response, PktSData);
    *Packet_GetLpL3Hdr(response) = (const LpL3){0};
    Packet_GetLpL3Hdr(response)->pitToken = token;

    return response;
}

static Packet *Producer_OnFileInfoInterest(Producer *producer, Packet *npkt,
                                           const LName name) {
    char *pathname =
        rte_malloc(NULL, XRDNDNDPDK_MAX_NAME_SIZE * sizeof(char), 0);
    Name_Decode_FilePath(name, PACKET_NAME_PREFIX_URI_FILEINFO_ENCODED_LEN,
                         pathname);

    ZF_LOGI("On FILEINFO Interest for file: %s", pathname);
    {
        int err = libfs_open(producer->fs, pathname);
        if (err != XRDNDNDPDK_ESUCCESS) {
            rte_free(pathname);
            return Producer_EncodeDataAsError(producer, npkt, err);
        }
    }

    struct stat *statbuf =
        (struct stat *)rte_malloc(NULL, sizeof(struct stat), 0);
    if (unlikely(NULL == statbuf)) {
        ZF_LOGF("Not enough memory to alloc struct stat");
        rte_free(pathname);
        return Producer_EncodeDataAsError(producer, npkt, XRDNDNDPDK_EFAILURE);
    }

    {
        int err = libfs_fstat(producer->fs, pathname, statbuf);
        if (err != XRDNDNDPDK_ESUCCESS) {
            rte_free(statbuf);
            rte_free(pathname);
            return Producer_EncodeDataAsError(producer, npkt, err);
        }
    }

    Packet *pkt = Producer_EncodeData(producer, npkt, sizeof(struct stat),
                                      (uint8_t *)statbuf);
    rte_free(statbuf);
    rte_free(pathname);
    return pkt;
}

static Packet *Producer_OnReadInterest(Producer *producer, Packet *npkt,
                                       const LName name) {
    char *pathname =
        rte_malloc(NULL, XRDNDNDPDK_MAX_NAME_SIZE * sizeof(char), 0);
    const uint8_t *segNumComp = RTE_PTR_ADD(
        name.value,
        Name_Decode_FilePath(name, PACKET_NAME_PREFIX_URI_READ_ENCODED_LEN,
                             pathname));

    ZF_LOGV("On READ Interest for file: %s", pathname);

    void *buf = rte_malloc(NULL, sizeof(uint8_t) * XRDNDNDPDK_PACKET_SIZE, 0);
    if (unlikely(NULL == buf)) {
        ZF_LOGF("Not enough memory to alloc read buffer of size %d",
                XRDNDNDPDK_PACKET_SIZE);
        rte_free(pathname);
        return Producer_EncodeDataAsError(producer, npkt, XRDNDNDPDK_EFAILURE);
    }

    assert(segNumComp[0] == TtSegmentNameComponent);
    uint64_t segNum = 0;
    Nni_Decode(segNumComp[1], RTE_PTR_ADD(segNumComp, 2), &segNum);

    int readRetCode =
        libfs_read(producer->fs, pathname, buf, XRDNDNDPDK_PACKET_SIZE, segNum);

    if (unlikely(readRetCode < 0)) {
        rte_free(buf);
        rte_free(pathname);
        return Producer_EncodeDataAsError(producer, npkt, -readRetCode);
    }

    Packet *pkt =
        Producer_EncodeData(producer, npkt, readRetCode, (uint8_t *)buf);
    rte_free(buf);
    rte_free(pathname);
    return pkt;
}

typedef Packet *(*OnInterest)(Producer *producer, Packet *npkt,
                              const LName name);

static const OnInterest onInterest[PACKET_MAX] = {
    [PACKET_FILEINFO] = Producer_OnFileInfoInterest,
    [PACKET_READ] = Producer_OnReadInterest,
};

static bool Producer_checkForRegisteredPrefix(LName name) {
    ZF_LOGD("Check for registered prefix");

    return (PACKET_NAME_PREFIX_URI_ENCODED_LEN <= name.length &&
            memcmp(PACKET_NAME_PREFIX_URI_ENCODED, name.value,
                   PACKET_NAME_PREFIX_URI_ENCODED_LEN) == 0);
}

static Packet *Producer_processInterest(Producer *producer, Packet *npkt) {
    ZF_LOGD("Processing Interest packet");

    const LName *name = (const LName *)&Packet_GetInterestHdr(npkt)->name;

    if (unlikely(!Producer_checkForRegisteredPrefix(*name))) {
        ZF_LOGW("Unsupported Interest prefix");
        return Nack_FromInterest(npkt, NackNoRoute);
    }

    PacketType pt = Name_Decode_PacketType(*name);

    if (unlikely(pt == PACKET_NOT_SUPPORTED)) {
        ZF_LOGW("Unsupported packet type");
        return Nack_FromInterest(npkt, NackNoRoute);
    }

    return onInterest[pt](producer, npkt, *name);
}

/**
 * @brief Main processing loop of current producer thread
 *
 * @param producer Pointer to Producer struct
 */
void Producer_Run(Producer *producer) {
    ZF_LOGI("Started producer instance on socket: %d lcore %d", rte_socket_id(),
            rte_lcore_id());

    struct rte_mbuf *rx[PRODUCER_MAX_BURST_SIZE];
    Packet *tx[PRODUCER_MAX_BURST_SIZE];

    while (ThreadStopFlag_ShouldContinue(&producer->stop)) {
        uint32_t nRx =
            PktQueue_Pop(&producer->rxQueue, rx, PRODUCER_MAX_BURST_SIZE,
                         rte_get_tsc_cycles())
                .count;

        if (unlikely(nRx == 0)) {
            rte_pause();
            continue;
        }

        uint16_t nTx = 0;
        for (uint16_t i = 0; i < nRx; ++i) {
            Packet *npkt = Packet_FromMbuf(rx[i]);
            NDNDPDK_ASSERT(Packet_GetType(npkt) == PktInterest);
            tx[nTx] = Producer_processInterest(producer, npkt);
            nTx += (int)(tx[nTx] != NULL);
        }

        Face_TxBurst(producer->face, tx, nTx);
    }
}
