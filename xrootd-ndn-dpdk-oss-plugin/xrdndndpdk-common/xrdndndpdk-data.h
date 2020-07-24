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

#ifndef XRDNDNDPDK_DATA_H
#define XRDNDNDPDK_DATA_H

#include "xrdndndpdk-utils.h"

/**
 * @brief Struct pointing to the Content Value in a Data packet
 *
 */
typedef struct PContent {
    uint8_t type;     // Content Type Value
    uint64_t length;  // Content Length Value
    uint16_t offset;  // Content offset in Data packet
    uint8_t *payload; // Payload from Data packet
} PContent;

void *Data_Encode_(struct rte_mbuf *m, uint16_t nameL, const uint8_t *nameV,
                   uint8_t contentType, uint32_t freshnessPeriod,
                   uint16_t contentL, const uint8_t *contentV);

__attribute__((nonnull)) static inline void *
Data_Encode(struct rte_mbuf *m, uint16_t nameL, const uint8_t *nameV,
            uint32_t freshnessPeriod, uint16_t contentL,
            const uint8_t *contentV) {
    // ContentType 0x00 (BLOB)
    // https://named-data.net/doc/NDN-packet-spec/current/types.html
    return Data_Encode_(m, nameL, nameV, 0x00, freshnessPeriod, contentL,
                        contentV);
}

__attribute__((nonnull)) static inline void *
Data_Encode_AppLNack(struct rte_mbuf *m, uint16_t nameL, const uint8_t *nameV,
                     uint32_t freshnessPeriod, uint16_t contentL,
                     const uint8_t *contentV) {
    // ContentType 0x03 (Application-level Nack)
    // https://named-data.net/doc/NDN-packet-spec/current/types.html
    return Data_Encode_(m, nameL, nameV, 0x03, freshnessPeriod, contentL,
                        contentV);
}

/**
 * @brief Decode Content offset in Data NDN packet format v0.3
 * https://named-data.net/doc/NDN-packet-spec/current/types.html
 *
 * @param content PContent struct
 * @param len Length of payload
 */
void Data_Decode(PContent *content, uint16_t len);

__attribute__((nonnull)) static inline bool Data_IsAppLNack(PContent *content) {
    // https://named-data.net/doc/NDN-packet-spec/current/types.html
    return content->type == 0x03;
}

#endif // XRDNDNDPDK_DATA_H