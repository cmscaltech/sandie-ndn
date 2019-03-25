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

#ifndef XRDNDN_PRODUCER_OPTIONS_HH
#define XRDNDN_PRODUCER_OPTIONS_HH

namespace xrdndnproducer {
/**
 * @brief XRootD NDN Producer options from command line
 *
 */
struct Options {
    /**
     * @brief Number of threads to process incoming Interest packets in parallel
     * on private IO Service Interest Manager's class member
     *
     */
    uint16_t threads = 32;

    /**
     * @brief Freshness period in seconds of all Interest packets composed by
     * Packager class
     *
     */
    uint64_t freshnessPeriod = 32;

    /**
     * @brief The time period in seconds as uint32_t when Garbage Collector
     * boost system timer from Interest Manager will be executed
     *
     */
    uint32_t gbTimePeriod = 60;

    /**
     * @brief The time period in seconds as std::chrono::seconds when Garbage
     * Collector boost system timer from Interest Manager will be executed
     *
     */
    std::chrono::seconds gbTimer;

    /**
     * @brief The time that Interest Manager class will keep track of an opened
     * file without being accessed anymore. Once the difference between last
     * access time on that specific file and the time when Garbage Collector
     * system timer is executed is greater than this value, the file will be
     * closed and the memory freed
     *
     */
    int64_t gbFileLifeTime = 60;

    /**
     * @brief Pre-cache an entire file before read operations on it will be
     * handled. This option is only for performance testing
     *
     */
    bool precacheFile = false;
};
} // namespace xrdndnproducer

#endif // XRDNDN_PRODUCER_OPTIONS_HH