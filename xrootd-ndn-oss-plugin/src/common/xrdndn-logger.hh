// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef XRDNDN_LOGGER_HH
#define XRDNDN_LOGGER_HH

#ifdef __MACH__
#pragma GCC diagnostic ignored "-Wunneeded-internal-declaration"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/logging.hpp>

namespace xrdndnconsumer {
/**
 * @brief Application is using ndn-log as logging module. This is the name of
 * the logging module for XRootD NDN Consumer. More information can be found at
 * https://named-data.net/doc/ndn-cxx/current/manpages/ndn-log.html
 *
 */
#define CONSUMER_LOGGER_PREFIX "xrdndnconsumer"

/**
 * @brief Initialize ndn-log module xrdndnconsumer
 *
 */
NDN_LOG_INIT(xrdndnconsumer);
} // namespace xrdndnconsumer

namespace xrdndnproducer {
/**
 * @brief Application is using ndn-log as logging module. This is the name of
 * the logging module for XRootD NDN Producer. More information can be found at
 * https://named-data.net/doc/ndn-cxx/current/manpages/ndn-log.html
 *
 */
#define PRODUCER_LOGGER_PREFIX "xrdndnproducer"

/**
 * @brief Initialize ndn-log module xrdndnproducer
 *
 */
NDN_LOG_INIT(xrdndnproducer);
} // namespace xrdndnproducer

#endif // XRDNDN_LOGGER_HH