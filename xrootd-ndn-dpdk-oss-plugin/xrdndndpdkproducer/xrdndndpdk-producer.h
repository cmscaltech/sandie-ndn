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

#pragma once

#ifndef XRDNDNDPDK_PRODUCER_H
#define XRDNDNDPDK_PRODUCER_H

#include "ndn-dpdk/csrc/dpdk/thread.h"
#include "ndn-dpdk/csrc/iface/face.h"
#include "ndn-dpdk/csrc/iface/pktqueue.h"

#include "../xrdndndpdkfilesystem/filesystem-c-api.h"

/**
 * @brief Producer struct
 *
 */
typedef struct Producer {
    PktQueue rxQueue;
    struct rte_mempool *dataMp;
    FaceID face;

    uint32_t freshnessPeriod;
    ThreadStopFlag stop;

    void *fs;
} Producer;

void Producer_Run(Producer *producer);

#endif // XRDNDNDPDK_PRODUCER_H
