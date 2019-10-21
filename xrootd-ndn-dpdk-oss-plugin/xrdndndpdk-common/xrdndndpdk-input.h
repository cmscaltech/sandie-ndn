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

#endif //XRDNDNDPDK_INPUT_H
