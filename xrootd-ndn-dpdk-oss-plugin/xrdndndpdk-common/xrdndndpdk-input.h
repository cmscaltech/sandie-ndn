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

#ifndef XRDNDNDPDK_INPUT_H
#define XRDNDNDPDK_INPUT_H

/// \file

#include "../../../ndn-dpdk/iface/face.h"

typedef struct InputEntry {
    struct rte_ring *consumerQueue; ///< queue toward consumer for Data and Nack
    struct rte_ring *producerQueue; ///< queue toward producer for Interest
} InputEntry;

/** \brief Input thread.
 */
typedef struct Input {
    uint16_t minFaceId;
    uint16_t maxFaceId;
    InputEntry entry[0];
} Input;

Input *Input_New(uint16_t minFaceId, uint16_t maxFaceId, unsigned numaSocket);

static InputEntry *Input_GetEntry(Input *input, uint16_t faceId) {
    if (faceId >= input->minFaceId && faceId <= input->maxFaceId) {
        return &input->entry[faceId - input->minFaceId];
    }
    return NULL;
}

void Input_FaceRx(FaceRxBurst *burst, void *input0);

#endif // XRDNDNDPDK_INPUT_H
