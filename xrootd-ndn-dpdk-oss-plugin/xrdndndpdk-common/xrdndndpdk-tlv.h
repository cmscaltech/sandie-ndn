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

#ifndef XRDNDNDPDK_TLV_H
#define XRDNDNDPDK_TLV_H

#include "xrdndndpdk-utils.h"

__attribute__((nonnull)) uint16_t
TlvDecoder_GenericNameComponentLength(const uint8_t *buf, uint16_t *off);

__attribute__((nonnull)) uint8_t *TlvEncoder_AppendTLV(struct rte_mbuf *m,
                                                       uint16_t len);

/**
 * @brief Append a TLV-TYPE or TLV-LENGTH number
 *
 * @param m Buffer to append number
 * @param n Number to append
 */
__attribute__((nonnull)) void TlvEncoder_AppendTL(struct rte_mbuf *m,
                                                  uint32_t n);

/**
 * @brief Prepend TLV-TYPE and TLV-LENGTH to mbuf.
 * @param m target mbuf, must have enough headroom.
 */
__attribute__((nonnull)) void
TlvEncoder_PrependTL(struct rte_mbuf *m, uint32_t type, uint32_t length);

#endif // XRDNDNDPDK_TLV_H