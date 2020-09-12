// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef XRDNDN_CONSUMER_OPTIONS_HH
#define XRDNDN_CONSUMER_OPTIONS_HH

#include <string>

namespace xrdndnconsumer {
/**
 * @brief Minimum Interest lifetime set from options
 *
 */
#define XRDNDN_MININTEREST_LIFETIME 1 // sec
/**
 * @brief Default Interest lifetime
 *
 */
#define XRDNDN_DEFAULT_INTEREST_LIFETIME 4 // sec
/**
 * @brief Maximum Interest lifetime set from options
 *
 */
#define XRDNDN_MAXINTEREST_LIFETIME 1024 // sec
/**
 * @brief Minimum fixed window size pipeline set from options
 *
 */
#define XRDNDN_MINPIPELINESZ 1 // Interests
/**
 * @brief Default fixed window size pipeline
 *
 */
#define XRDNDN_DEFAULT_PIPELINESZ 64 // Interests
/**
 * @brief Maximum fixed window size pipeline set from options
 *
 */
#define XRDNDN_MAXPIPELINESZ 512 // Interests

/**
 * @brief XRootD NDN Consumer instance options
 *
 */
struct Options {
    /**
     * @brief The number of concurrent Interest packets expressed at one time in
     * the fixed window size Pipeline
     *
     */
    size_t pipelineSize = XRDNDN_DEFAULT_PIPELINESZ;

    /**
     * @brief The Interest life time expressed in seconds
     *
     */
    size_t interestLifetime = XRDNDN_DEFAULT_INTEREST_LIFETIME;

    /**
     * @brief Log level: TRACE DEBUG INFO WARN ERROR FATAL. More information is
     * available at:
     * https://named-data.net/doc/ndn-cxx/current/manpages/ndn-log.html
     *
     */
    std::string logLevel = "INFO";
};
} // namespace xrdndnconsumer

#endif // XRDNDN_CONSUMER_OPTIONS_HH