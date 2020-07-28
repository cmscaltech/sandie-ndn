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

#ifndef XRDNDNDPDK_CONSUMER_RX_H
#define XRDNDNDPDK_CONSUMER_RX_H

#include "ndn-dpdk/csrc/dpdk/thread.h"
#include "ndn-dpdk/csrc/iface/pktqueue.h"

#include "../xrdndndpdk-common/xrdndndpdk-data.h"
#include "../xrdndndpdk-common/xrdndndpdk-name.h"

typedef void (*onContentCallback)(uint64_t, uint64_t);
typedef void (*onErrorCallback)(uint64_t);

void onContentCallback_Go(uint64_t value, uint64_t segmentNum);
void onErrorCallback_Go(uint64_t errorCode);

/**
 * @brief Rx counters
 *
 */
typedef struct CountersRx {
    uint64_t nData;
    uint64_t nNack;
    uint64_t nBytes;
} CountersRx;

/**
 */
typedef struct ConsumerRx {
    PktQueue rxQueue;
    ThreadStopFlag stop;
    onContentCallback onContent;
    onErrorCallback onError;

    CountersRx cnt;
} ConsumerRx;

__attribute__((nonnull)) int ConsumerRx_Run(ConsumerRx *cr);

__attribute__((unused, nonnull)) static void
ConsumerRx_RegisterGoCallbacks(ConsumerRx *cr) {
    cr->onContent = onContentCallback_Go;
    cr->onError = onErrorCallback_Go;
}

__attribute__((unused, nonnull)) static void
ConsumerRx_ClearCounters(ConsumerRx *cr) {
    cr->cnt.nData = 0;
    cr->cnt.nNack = 0;
    cr->cnt.nBytes = 0;
}

#endif // XRDNDNDPDK_CONSUMER_RX_H
