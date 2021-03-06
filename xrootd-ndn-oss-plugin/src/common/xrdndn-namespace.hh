// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
//
// Author: Catalin Iordache <catalin.iordache@cern.ch>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

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
#define XRDNDN_MAX_PAYLOAD_SIZE 6144 // 6KB

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