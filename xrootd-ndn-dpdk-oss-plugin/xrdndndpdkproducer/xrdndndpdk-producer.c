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

#include "xrdndndpdk-producer.h"

INIT_ZF_LOG(Xrdndndpdkproducer);

/**
 * @brief Check if Interest Name prefix is recognized by this producer
 *
 * @param name Interest Name as LName struct
 * @return true Interest Name prefix is recognized by producer
 * @return false Interest Name prefix is not recognized by producer
 */
static bool Producer_checkForRegisteredPrefix(LName name) {
    ZF_LOGD("Check for registered prefix");

    return (XRDNDNDPDK_SYCALL_PREFIX_ENCODE_SIZE <= name.length &&
            memcmp(XRDNDNDPDK_SYCALL_PREFIX_ENCODE, name.value,
                   XRDNDNDPDK_SYCALL_PREFIX_ENCODE_SIZE) == 0);
}

/**
 * @brief Returns Nack Data packet type
 *
 * @param npkt Packet (Interest) received from consumer
 * @param nackReason Nack reason
 * @return Packet* Data packet Nack type
 */
static Packet *Producer_getPacket_Nack(Packet *npkt, NackReason nackReason) {
    ZF_LOGD("Get Nack Packet with reason: %d", nackReason);

    MakeNack(npkt, nackReason);
    return npkt;
}

/**
 * @brief Create Packet to send over NDN network
 *
 * @param producer Pointer to Producer struct
 * @param npkt Packet received over NDN network
 * @param type Packet L3 type. L3PktType_MAX type used for application level
 * Nack
 * @param length Content length
 * @param content Content of packet
 * @return Packet* Packet to send over NDN network
 */
static Packet *Producer_getPacket(Producer *producer, Packet *npkt,
                                  L3PktType type, uint64_t length,
                                  uint8_t *content) {
    ZF_LOGD("Get packet type: %d with content length: %d", type, length);

    uint64_t token = Packet_GetLpL3Hdr(npkt)->pitToken;
    const LName name = *(const LName *)&Packet_GetInterestHdr(npkt)->name;

    struct rte_mbuf *pkt = rte_pktmbuf_alloc(producer->dataMp);
    if (unlikely(pkt == NULL)) {
        ZF_LOGW("dataMp-full");
        rte_pktmbuf_free(Packet_ToMbuf(npkt));
        return NULL;
    }

    EncodeData(pkt, name, lnameStub, producer->freshnessPeriod, length,
               content);
    rte_pktmbuf_free(Packet_ToMbuf(npkt));

    Packet *response = Packet_FromMbuf(pkt);
    Packet_SetL2PktType(response, L2PktType_None);
    Packet_InitLpL3Hdr(response)->pitToken = token;
    Packet_SetL3PktType(response, type);
    return response;
}

/**
 * @brief Get Data packet containing an uint64_t as content
 *
 * @param producer Pointer to Producer struct
 * @param npkt Packet received over NDN network
 * @param type Packet L3 type. L3PktType_MAX type used for application level
 * Nack
 * @param content Content of packet
 * @return Packet* Data packet
 */
static Packet *Producer_getNonNegativeIntegerPacket(Producer *producer,
                                                    Packet *npkt,
                                                    L3PktType type,
                                                    uint64_t content) {
    ZF_LOGD("Get Data Packet with uint64_t as content");

    uint64_t v = rte_cpu_to_be_64(content);
    return Producer_getPacket(producer, npkt, L3PktType_Data, sizeof(uint64_t),
                              (uint8_t *)&v);
}

/**
 * @brief Process Interest for file open system call
 *
 * @param producer Pointer to Producer struct
 * @param npkt Packet received over NDN network
 * @param name Name from packet
 * @return Packet* Response packet to Interest for file open system call
 */
static Packet *Producer_onOpenInterest(Producer *producer, Packet *npkt,
                                       const LName name) {
    char *path = lnameGetFilePath(
        name, XRDNDNDPDK_SYCALL_PREFIX_OPEN_ENCODE_SIZE, false);

    ZF_LOGI("Received open Interest for file: %s", path);

    uint64_t openReturnCode = 0;
    int fd;
    if ((fd = open(path, O_RDONLY)) == -XRDNDNDPDK_EFAILURE) {
        openReturnCode = errno;
    } else {
        close(fd);
    }

    ZF_LOGI("Open file: %s with error code: %d (%s)", path, openReturnCode,
            strerror(openReturnCode));

    rte_free(path);
    return Producer_getNonNegativeIntegerPacket(producer, npkt, L3PktType_Data,
                                                openReturnCode);
}

/**
 * @brief Process Interest for file stat system call
 *
 * @param producer Pointer to Producer struct
 * @param npkt Packet received over NDN network
 * @param name Name from packet
 * @return Packet* Response packet to Interest for file stat system call
 */
static Packet *Producer_onFstatInterest(Producer *producer, Packet *npkt,
                                        const LName name) {
    char *path = lnameGetFilePath(
        name, XRDNDNDPDK_SYCALL_PREFIX_FSTAT_ENCODE_SIZE, false);

    ZF_LOGI("Received fstat Interest for file: %s", path);

    struct stat *statbuf =
        (struct stat *)rte_malloc(NULL, sizeof(struct stat), 0);

    if (unlikely(NULL == statbuf)) {
        ZF_LOGE("Not enough memory");
    }

    if (unlikely(stat(path, statbuf) == -XRDNDNDPDK_EFAILURE)) {
        int fstatRetCode = errno;

        ZF_LOGW("Fstat for file: %s failed with error code: %d (%s)", path,
                fstatRetCode, strerror(fstatRetCode));

        return Producer_getNonNegativeIntegerPacket(
            producer, npkt, L3PktType_MAX, fstatRetCode);
    }

    ZF_LOGI("Fstat file: %s with success", path);

    Packet *pkt = Producer_getPacket(producer, npkt, L3PktType_Data,
                                     sizeof(struct stat), (uint8_t *)statbuf);
    rte_free(path);
    rte_free(statbuf);
    return pkt;
}

/**
 * @brief Process Interest for read file system call
 *
 * @param producer Producer struct pointer
 * @param npkt Packet received over NDN network
 * @param name Name from packet
 * @return Packet* Response packet to Interest for read file system call
 */
static Packet *Producer_onReadInterest(Producer *producer, Packet *npkt,
                                       const LName name) {
    char *path =
        lnameGetFilePath(name, XRDNDNDPDK_SYCALL_PREFIX_READ_ENCODE_SIZE, true);

    ZF_LOGD("Received read Interest for file: %s", path);

    uint64_t off = lnameGetSegmentNumber(name);
    ZF_LOGI("Read %dB @%d from file: %s", XRDNDNDPDK_PACKET_SIZE, off, path);

    int readRetCode;
    int fd;

    if (unlikely((fd = open(path, O_RDONLY)) == -XRDNDNDPDK_EFAILURE)) {
        readRetCode = errno;

        ZF_LOGI("Open file: %s with error code: %d (%s)", path, readRetCode,
                strerror(readRetCode));

        return Producer_getNonNegativeIntegerPacket(producer, npkt,
                                                    L3PktType_MAX, readRetCode);
    }

    void *buf = rte_malloc(NULL, sizeof(uint8_t) * XRDNDNDPDK_PACKET_SIZE, 0);
    if (unlikely(NULL == buf)) {
        ZF_LOGE("Not enough memory");

        rte_free(path);
        close(fd);

        return Producer_getNonNegativeIntegerPacket(
            producer, npkt, L3PktType_MAX, XRDNDNDPDK_EFAILURE);
    }

    readRetCode = pread(fd, buf, XRDNDNDPDK_PACKET_SIZE, off);
    if (unlikely(readRetCode == -XRDNDNDPDK_EFAILURE)) {
        readRetCode = errno;
        ZF_LOGW("Read from file: %s @%d failed with error code: %d (%s)", path,
                off, readRetCode, strerror(readRetCode));

        close(fd);
        rte_free(path);
        rte_free(buf);

        return Producer_getNonNegativeIntegerPacket(producer, npkt,
                                                    L3PktType_MAX, readRetCode);
    }

    close(fd);

    ZF_LOGD("Read from file: %s @%d %dB", path, off, readRetCode);

    Packet *pkt;
    if (likely(readRetCode != 0)) {
        pkt = Producer_getPacket(producer, npkt, L3PktType_Data, readRetCode,
                                 (uint8_t *)buf);
    } else {
        pkt = Producer_getNonNegativeIntegerPacket(producer, npkt,
                                                   L3PktType_MAX, readRetCode);
    }

    rte_free(path);
    rte_free(buf);
    return pkt;
}

/**
 * @brief Function prototype for processing Interest packets
 *
 */
typedef Packet *(*OnSystemCallInterest)(Producer *producer, Packet *npkt,
                                        const LName name);

static const OnSystemCallInterest onSystemCallInterest[SYSCALL_MAX_ID] = {
    [SYSCALL_OPEN_ID] = Producer_onOpenInterest,
    [SYSCALL_FSTAT_ID] = Producer_onFstatInterest,
    [SYSCALL_READ_ID] = Producer_onReadInterest,
};

/**
 * @brief Function to process each Interest
 *
 * @param producer Pointer to Producer struct
 * @param npkt Packet received over NDN network
 * @return Packet* Response packet to received Interest
 */
static Packet *Producer_processInterest(Producer *producer, Packet *npkt) {
    ZF_LOGD("Processing new Interest packet");

    const LName name = *(const LName *)&Packet_GetInterestHdr(npkt)->name;

    if (unlikely(!Producer_checkForRegisteredPrefix(name))) {
        ZF_LOGW("Interest prefix for Name: %s not matching prefix: %s",
                name.value, XRDNDNDPDK_SYSCALL_PREFIX_URI);
        return Producer_getPacket_Nack(npkt, NackReason_NoRoute);
    }

    SystemCallId sid = lnameGetSystemCallId(name);

    if (unlikely(sid == SYSCALL_NOT_FOUND)) {
        ZF_LOGW("Unrecognized file system call for Interest Name: %s", name);
        return Producer_getPacket_Nack(npkt, NackReason_Unspecified);
    }

    return onSystemCallInterest[sid](producer, npkt, name);
}

/**
 * @brief Main processing loop of current producer thread
 *
 * @param producer Pointer to Producer struct
 */
void Producer_Run(Producer *producer) {
    ZF_LOGI("Started producer instance on socket: %d lcore %d", rte_socket_id(),
            rte_lcore_id());

    Packet *rx[PRODUCER_MAX_BURST_SIZE];
    Packet *tx[PRODUCER_MAX_BURST_SIZE];

    while (ThreadStopFlag_ShouldContinue(&producer->stop)) {
        uint16_t nRx = rte_ring_sc_dequeue_burst(producer->rxQueue, (void **)rx,
                                                 PRODUCER_MAX_BURST_SIZE, NULL);
        uint16_t nTx = 0;
        for (uint16_t i = 0; i < nRx; ++i) {
            Packet *npkt = rx[i];
            assert(Packet_GetL3PktType(npkt) == L3PktType_Interest);
            tx[nTx] = Producer_processInterest(producer, npkt);
            nTx += (tx[nTx] != NULL);
        }

        Face_TxBurst(producer->face, tx, nTx);
    }
}
