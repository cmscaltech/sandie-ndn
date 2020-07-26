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

#ifndef XRDNDNDPDK_UTILS_H
#define XRDNDNDPDK_UTILS_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ndn-dpdk/csrc/core/logger.h"
#include "ndn-dpdk/csrc/ndni//nni.h"
#include "ndn-dpdk/csrc/ndni/name.h"

#include "xrdndndpdk-namespace.h"

__attribute__((unused)) static void copyFromC(uint8_t *dst, uint16_t dst_off,
                                              uint8_t *src, uint16_t src_off,
                                              uint16_t count) {
    memcpy(&dst[dst_off], &src[src_off], count);
}

#endif // XRDNDNDPDK_UTILS_H
