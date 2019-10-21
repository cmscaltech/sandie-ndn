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

#ifndef XRDNDNDPDK_CONSUMER_RX_H
#define XRDNDNDPDK_CONSUMER_RX_H

#include "../../../ndn-dpdk/core/running_stat/running-stat.h"
#include "../../../ndn-dpdk/dpdk/thread.h"

#include "../xrdndndpdk-common/xrdndndpdk-utils.h"


#define CONSUMER_RX_BURST_SIZE 64

typedef void(*onDataCallback)(struct PContent*);
typedef void(*onNonNegativeIntegerDataCallback)(uint64_t);

typedef void(*onErrorCallback)(void);

void onOpenDataCallback_Go(uint64_t openRetCode);
void onErrorCallback_Go();

/**
 */
typedef struct ConsumerRx {
  struct rte_ring* rxQueue;
  ThreadStopFlag stop;

  uint64_t nData;
  uint64_t nNacks;

  onDataCallback onData;
  onNonNegativeIntegerDataCallback onOpenData;
  onErrorCallback onError;
} ConsumerRx;

void
ConsumerRx_Run(ConsumerRx* cr);

static void registerRxCallbacks(ConsumerRx* cr) {
    //cr->onData = onDataCallback_Go;
    cr->onOpenData = onOpenDataCallback_Go;
    cr->onError = onErrorCallback_Go;
}

#endif // XRDNDNDPDK_CONSUMER_RX_H