#include "xrdndndpdk-input.h"

Input *Input_New(uint16_t minFaceId, uint16_t maxFaceId, unsigned numaSocket) {
    size_t size =
        sizeof(Input) + sizeof(InputEntry) * (maxFaceId - minFaceId + 1);
    Input *input = (Input *)rte_zmalloc_socket("Input", size, 0, numaSocket);
    input->minFaceId = minFaceId;
    input->maxFaceId = maxFaceId;
    return input;
}

void Input_FaceRx(FaceRxBurst *burst, void *input0) {
    Input *input = (Input *)input0;

#define DISPATCH_TO(queueName)                                                 \
    do {                                                                       \
        struct rte_mbuf *m = Packet_ToMbuf(npkt);                              \
        uint16_t faceId = m->port;                                             \
        if (likely(faceId >= input->minFaceId &&                               \
                   faceId <= input->maxFaceId)) {                              \
            struct rte_ring *queue =                                           \
                input->entry[faceId - input->minFaceId].queueName;             \
            if (likely(queue != NULL) &&                                       \
                likely(rte_ring_sp_enqueue(queue, npkt) == 0)) {               \
                break;                                                         \
            }                                                                  \
        }                                                                      \
        rte_pktmbuf_free(m);                                                   \
    } while (false)

    for (uint16_t i = 0; i < burst->nInterests; ++i) {
        Packet *npkt = FaceRxBurst_GetInterest(burst, i);
        DISPATCH_TO(producerQueue);
    }
    for (uint16_t i = 0; i < burst->nData; ++i) {
        Packet *npkt = FaceRxBurst_GetData(burst, i);
        DISPATCH_TO(consumerQueue);
    }
    for (uint16_t i = 0; i < burst->nNacks; ++i) {
        Packet *npkt = FaceRxBurst_GetNack(burst, i);
        DISPATCH_TO(consumerQueue);
    }

#undef DISPATCH_TO
}
