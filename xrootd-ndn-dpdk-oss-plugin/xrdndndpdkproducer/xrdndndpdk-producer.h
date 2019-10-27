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

#ifndef XRDNDNDPDK_PRODUCER_H
#define XRDNDNDPDK_PRODUCER_H

#include "../../../ndn-dpdk/core/pcg_basic.h"
#include "../../../ndn-dpdk/dpdk/thread.h"
#include "../../../ndn-dpdk/iface/face.h"
#include "../../../ndn-dpdk/ndn/encode-data.h"

#include "../xrdndndpdk-common/xrdndndpdk-utils.h"

/**
 * @brief Producer struct
 *
 */
typedef struct Producer {
    struct rte_ring *rxQueue;
    struct rte_mempool *dataMp;
    FaceId face;

    uint32_t freshnessPeriod;

    ThreadStopFlag stop;
} Producer;

void Producer_Run(Producer *producer);

#endif // XRDNDNDPDK_PRODUCER_H
