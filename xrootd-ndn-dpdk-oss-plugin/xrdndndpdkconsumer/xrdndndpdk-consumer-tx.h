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

#include "../../../ndn-dpdk/dpdk/thread.h"
#include "../../../ndn-dpdk/iface/face.h"
#include "../../../ndn-dpdk/ndn/encode-interest.h"

typedef void (*onErrorCallback)(uint64_t);

void onErrorCallback_Go(uint64_t errorCode);

/**
 * @brief Consumer Tx structure
 *
 */
typedef struct ConsumerTx {
    FaceId face;
    uint16_t interestMbufHeadroom;
    ThreadStopFlag stop;

    struct rte_mempool *interestMp; // mempool for Interests
    NonceGen nonceGen;
    uint8_t prefixBuffer[NAME_MAX_LENGTH];

    InterestTemplate tpl;
    uint8_t tplPrepareBuffer[64];

    onErrorCallback onError;
} ConsumerTx;

void ConsumerTx_sendInterest(ConsumerTx *ct, struct LName *suffix);

static void registerTxCallbacks(ConsumerTx *ct) {
    ct->onError = onErrorCallback_Go;
}

#endif // XRDNDNDPDK_CONSUMER_TX_H
