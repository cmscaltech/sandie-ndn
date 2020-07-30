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

#ifndef XRDNDNDPDK_NAMESPACE_H
#define XRDNDNDPDK_NAMESPACE_H

#include <stdint.h>

/**
 * @brief Maximum Data packet payload size
 *
 */
#define XRDNDNDPDK_MAX_PAYLOAD_SIZE 6144

/**
 * @brief Maximum Name length
 *
 */
#define XRDNDNDPDK_MAX_NAME_SIZE 2048

/**
 * @brief Consumer TX thread maximum burst size
 *
 */
#define CONSUMER_TX_MAX_BURST_SIZE 32

/**
 * @brief Success error code
 *
 */
#define XRDNDNDPDK_ESUCCESS 0

/**
 * @brief Fail error code
 *
 */
#define XRDNDNDPDK_EFAILURE 1

/**
 * @brief Name prefix for all Packets expressed in SANDIE project
 *
 */
#define PACKET_NAME_PREFIX_URI "/ndn/xrootd"

/**
 * @brief NDN Name v0.3 TLV format for "/ndn/xrootd"
 *
 */
static const uint8_t PACKET_NAME_PREFIX_URI_ENCODED[] = {
    0x08, 0x03, 0x6E, 0x64, 0x6E, 0x08, 0x06,
    0x78, 0x72, 0x6F, 0x6F, 0x74, 0x64};

/**
 * @brief NDN Name v0.3 TLV "/ndn/xrootd" format length
 *
 */
#define PACKET_NAME_PREFIX_URI_ENCODED_LEN 13

/**
 * @brief Name prefix component for FILEINFO Packets
 *
 */
#define PACKET_NAME_PREFIX_URI_FILEINFO "/ndn/xrootd/fileinfo"

/**
 * @brief NDN Name v0.3 TLV format for "/ndn/xrootd/fileinfo"
 *
 */
static const uint8_t PACKET_NAME_PREFIX_URI_FILEINFO_ENCODED[] = {
    0x08, 0x03, 0x6E, 0x64, 0x6E, 0x08, 0x06, 0x78, 0x72, 0x6F, 0x6F, 0x74,
    0x64, 0x08, 0x08, 0x66, 0x69, 0x6C, 0x65, 0x69, 0x6E, 0x66, 0x6F};

/**
 * @brief NDN Name v0.3 TLV "/ndn/xrootd/fileinfo" format size
 *
 */
#define PACKET_NAME_PREFIX_URI_FILEINFO_ENCODED_LEN 23

/**
 * @brief Name prefix component for READ Packets
 *
 */
#define PACKET_NAME_PREFIX_URI_READ "/ndn/xrootd/read"

/**
 * @brief NDN Name v0.3 TLV format for "/ndn/xrootd/read"
 *
 */
static const uint8_t PACKET_NAME_PREFIX_URI_READ_ENCODED[] = {
    0x08, 0x03, 0x6E, 0x64, 0x6E, 0x08, 0x06, 0x78, 0x72, 0x6F,
    0x6F, 0x74, 0x64, 0x08, 0x04, 0x72, 0x65, 0x61, 0x64};

/**
 * @brief NDN Name v0.3 TLV "/ndn/xrootd/read" format size
 *
 */
#define PACKET_NAME_PREFIX_URI_READ_ENCODED_LEN 19

/**
 * @brief List of Packet types
 *
 */
typedef enum PacketType {
    PACKET_FILEINFO = 0,
    PACKET_READ,

    PACKET_MAX,
    PACKET_NOT_SUPPORTED,
} PacketType;

#endif // XRDNDNDPDK_NAMESPACE_H
