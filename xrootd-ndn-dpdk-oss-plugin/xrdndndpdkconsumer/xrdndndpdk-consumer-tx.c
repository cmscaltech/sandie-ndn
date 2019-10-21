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

#include "../../../ndn-dpdk/core/logger.h"

#include "xrdndndpdk-consumer-tx.h"

INIT_ZF_LOG(Xrdndndpdkconsumer);

/**
 * @brief Send Open Interest over NDN
 *
 * @param ct ConsumerTx struct pointer
 * @param suffix Suffix of Name
 */
void ConsumerTx_SendOpenInterest(ConsumerTx *ct, struct LName *suffix) {
    ZF_LOGD("Sending Interest for file open systemcall");

    struct rte_mbuf *openInterestPkt = rte_pktmbuf_alloc(ct->interestMp);
    if (NULL == openInterestPkt) {
        ZF_LOGE("interestMp-full");
        ct->onError();
        return;
    }

    openInterestPkt->data_off = ct->interestMbufHeadroom;
    EncodeInterest(openInterestPkt, &ct->tpl, ct->tplPrepareBuffer, *suffix,
                   NonceGen_Next(&ct->nonceGen), 0, NULL);

    Packet_SetL3PktType(Packet_FromMbuf(openInterestPkt), L3PktType_Interest);
    Face_Tx(ct->face, Packet_FromMbuf(openInterestPkt));
}
