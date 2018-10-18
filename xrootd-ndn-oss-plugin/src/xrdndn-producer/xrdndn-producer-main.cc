/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018 California Institute of Technology                        *
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

#include <iostream>
#include <ndn-cxx/face.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>

#include "../common/xrdndn-logger.hh"
#include "xrdndn-producer.hh"

namespace xrdndnproducer {

#define NUM_FACE_WORKERS 32

static boost::asio::io_service ioService;
static boost::thread_group threads;

int main() {
    auto addWorkerThreads = [&]() {
        NDN_LOG_INFO("[main]: "
                     << NUM_FACE_WORKERS
                     << " threads were added for processing face events.");
        for (size_t i = 0; i < NUM_FACE_WORKERS; ++i)
            threads.create_thread(
                std::bind(static_cast<size_t (boost::asio::io_service::*)()>(
                              &boost::asio::io_service::run),
                          &ioService));
    };

    auto joinWorkerThreads = [&]() {
        NDN_LOG_INFO("[main]: Joining " << NUM_FACE_WORKERS
                                        << " processing threads from face.");
        threads.join_all();
    };

    addWorkerThreads();
    ndn::Face face(ioService);
    try {
        Producer producer(face);
        face.processEvents();
    } catch (const std::exception &e) {
        NDN_LOG_ERROR("[main]: " << e.what());
        joinWorkerThreads();
        return 1;
    }

    joinWorkerThreads();
    return 0;
}
} // namespace xrdndnproducer

int main(int, char **) { return xrdndnproducer::main(); }
