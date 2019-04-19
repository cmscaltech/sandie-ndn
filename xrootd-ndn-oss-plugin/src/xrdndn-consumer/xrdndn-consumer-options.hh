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

#ifndef XRDNDN_CONSUMER_OPTIONS_HH
#define XRDNDN_CONSUMER_OPTIONS_HH

namespace xrdndnconsumer {
/**
 * @brief XRootD NDN Consumer instance options
 *
 */
struct Options {
    /**
     * @brief Path to file to be retrieved over NDN network. One Consumer
     * instance object per file
     *
     */
    std::string path = "";

    /**
     * @brief The number of concurrent Interest packets expressed at one time in
     * the fixed window size Pipeline
     *
     */
    size_t pipelineSize = 64;

    // TOOD: Log level
    // TODO: Interest life time in seconds
    // TODO: Pipeline size
};
} // namespace xrdndnconsumer

#endif // XRDNDN_CONSUMER_OPTIONS_HH