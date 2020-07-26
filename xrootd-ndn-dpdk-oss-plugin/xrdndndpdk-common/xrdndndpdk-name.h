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

#ifndef XRDNDNDPDK_NAME_H
#define XRDNDNDPDK_NAME_H

#include "xrdndndpdk-utils.h"

/**
 * @brief Decode filepath from LName
 *
 * @param name Packet Name as LName struct
 * @param off Offset to the first Name component of filepath; skip prefix
 * @param filepath output
 * @return uint16_t offset to end of filepath in LName
 */
__attribute__((nonnull)) uint16_t
Name_Decode_FilePath(const LName name, uint16_t off, char *filepath);

/**
 * @brief Decode filepath length from LName
 *
 * @param name Packet Name as LName struct
 * @param off Offset to the first Name component of filepath; skip prefix
 * @return uint16_t Filepath encoded length
 */
__attribute__((nonnull)) uint16_t Name_Decode_FilePathLength(const LName name,
                                                             uint16_t off);

/**
 * @brief Decode packet type from LName
 *
 * @param name Packet Name
 * @return PacketType Packet type
 */
__attribute__((nonnull)) PacketType Name_Decode_PacketType(const LName name);

#endif // XRDNDNDPDK_NAME_H
