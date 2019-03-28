/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
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

#ifndef XRDNDN_NAMESPACE_HH
#define XRDNDN_NAMESPACE_HH

#include <ndn-cxx/name.hpp>

namespace xrdndn {
/**
 * @brief Success error code
 *
 */
#define XRDNDN_ESUCCESS 0

/**
 * @brief Fail error code
 *
 */
#define XRDNDN_EFAILURE -1

/**
 * @brief Default Data packet size in XRootD NDN based file system plugin
 *
 */
#define XRDNDN_MAX_NDN_PACKET_SIZE 7168

/**
 * @brief Name prefix for all Interest packets expressed by Consumer
 *
 */
static const ndn::Name SYS_CALLS_PREFIX_URI("/ndn/xrootd/");
/**
 * @brief Keep the number of components in each prefix
 *
 */
static const uint8_t SYS_CALLS_PREFIX_LEN = SYS_CALLS_PREFIX_URI.size() + 1;

/**
 * @brief Name filter for open system call Interest packet
 *
 */
static const ndn::Name SYS_CALL_OPEN_PREFIX_URI("/ndn/xrootd/open/");
/**
 * @brief Name filter for fstat system call Interest packet
 *
 */
static const ndn::Name SYS_CALL_FSTAT_PREFIX_URI("/ndn/xrootd/fstat/");
/**
 * @brief Name filter for read system call Interest packet
 *
 */
static const ndn::Name SYS_CALL_READ_PREFIX_URI("/ndn/xrootd/read/");
} // namespace xrdndn

#endif // XRDNDN_NAMESPACE_HH