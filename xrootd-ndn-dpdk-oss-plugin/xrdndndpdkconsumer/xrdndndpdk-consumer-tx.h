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

#ifndef XRDNDNDPDK_CONSUMER_TX_H
#define XRDNDNDPDK_CONSUMER_TX_H

#include "ndn-dpdk/csrc/dpdk/thread.h"
#include "ndn-dpdk/csrc/iface/face.h"
#include "ndn-dpdk/csrc/iface/pktqueue.h"

#include "../xrdndndpdk-common/xrdndndpdk-utils.h"

typedef void (*onErrorCallback)(uint64_t);
void onErrorCallback_Go(uint64_t errorCode);

/**
 * @brief Tx counters
 *
 */
typedef struct CountersTx {
    uint64_t nInterest;
} CountersTx;

/**
 * @brief
 *
 */
typedef struct ConsumerTxRequest {
    PacketType pt;
    uint16_t npkts;
    uint64_t off;
    uint16_t nameL;
    uint8_t nameV[XRDNDNDPDK_MAX_NAME_SIZE];
} ConsumerTxRequest;

/**
 * @brief Consumer Tx structure
 *
 */
typedef struct ConsumerTx {
    FaceID face;
    ThreadStopFlag stop;
    struct rte_mempool *interestMp; // mempool for Interests
    struct rte_ring *requestQueue;

    NonceGen nonceGen;
    InterestTemplate fileInfoPrefixTpl;
    InterestTemplate readPrefixTpl;

    onErrorCallback onError;
    CountersTx cnt;
} ConsumerTx;

__attribute__((nonnull)) int ConsumerTx_Run(ConsumerTx *ct);

__attribute__((unused, nonnull)) inline void
ConsumerTx_RegisterGoCallbacks(ConsumerTx *ct) {
    ct->onError = onErrorCallback_Go;
}

__attribute__((unused, nonnull)) static void
ConsumerTx_ClearCounters(ConsumerTx *ct) {
    ct->cnt.nInterest = 0;
}

#endif // XRDNDNDPDK_CONSUMER_TX_H
