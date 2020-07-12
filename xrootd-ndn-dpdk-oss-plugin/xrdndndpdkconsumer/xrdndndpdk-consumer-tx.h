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

#ifndef XRDNDNDPDK_CONSUMER_TX_H
#define XRDNDNDPDK_CONSUMER_TX_H

#include "ndn-dpdk/container/pktqueue/queue.h"
#include "ndn-dpdk/dpdk/thread.h"
#include "ndn-dpdk/iface/face.h"

#include "../xrdndndpdk-common/xrdndndpdk-utils.h"

typedef void (*onErrorCallback)(uint64_t);
void onErrorCallback_Go(uint64_t errorCode);

/**
 * @brief Consumer Tx structure
 *
 */
typedef struct ConsumerTx {
    FaceId face;
    ThreadStopFlag stop;
    struct rte_mempool *interestMp; // mempool for Interests
    NonceGen nonceGen;
    InterestTemplate prefixTpl;
    InterestTemplate readPrefixTpl;
    onErrorCallback onError;
    uint64_t nInterests;
} ConsumerTx;

void ConsumerTx_resetCounters(ConsumerTx *ct);
void ConsumerTx_sendInterest(ConsumerTx *ct, struct LName *suffix);
void ConsumerTx_sendInterests(ConsumerTx *ct, struct LName *path, uint64_t off,
                              uint16_t n);

inline void registerTxCallbacks(ConsumerTx *ct) {
    ct->onError = onErrorCallback_Go;
}

#endif // XRDNDNDPDK_CONSUMER_TX_H
