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
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/thread/thread.hpp>

#include "../common/xrdndn-logger.hh"
#include "xrdndn-producer.hh"

namespace xrdndnproducer {

#define NUM_FACE_WORKERS 32

static boost::asio::io_service ioService;
static boost::thread_group threads;

int run(const Options &opts) {
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
        Producer producer(face, opts);
        face.processEvents();
    } catch (const std::exception &e) {
        NDN_LOG_ERROR("[main]: " << e.what());
        joinWorkerThreads();
        return 2;
    }

    joinWorkerThreads();
    return 0;
}

static void usage(std::ostream &os, const std::string &programName,
                  const boost::program_options::options_description &desc) {
    os << "Usage: export NDN_LOG=\"xrdndnproducer*=INFO\" && " << programName
       << " [options]\nNote: This application can be run without "
          "arguments.\n\n"
       << desc;
}

static void info(const Options &opts) {
    NDN_LOG_INFO(
        "\nThe suitable NDN Producer for the NDN based filesystem plugin for "
        "XRootD.\nDeveloped by Caltech@CMS.\n\nSelected Options: Pre-cache "
        "files: "
        << opts.precacheFile << ", Signer Type: " << opts.signerType << "\n");
}

int main(int argc, char **argv) {
    std::string programName = argv[0];

    Options opts;

    boost::program_options::options_description description("Options");
    description.add_options()("help,h", "Print this help message and exit")(
        "precache-files,P",
        boost::program_options::bool_switch(&opts.precacheFile),
        "Precache files in memory before responding to read interests")(
        "signer-type,s",
        boost::program_options::value<uint32_t>(&opts.signerType)
            ->default_value(opts.signerType),
        "Signer type: 0, 1, 2, 3 or 4. See more information at "
        "https://named-data.net/doc/NDN-packet-spec/current/signature.html");

    boost::program_options::variables_map vm;

    try {
        boost::program_options::store(
            boost::program_options::command_line_parser(argc, argv)
                .options(description)
                .run(),
            vm);
        boost::program_options::notify(vm);
    } catch (const boost::program_options::error &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 2;
    } catch (const boost::bad_any_cast &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 2;
    }

    if (vm.count("help") > 0) {
        usage(std::cout, programName, description);
        return 0;
    }

    if (opts.signerType > 4) {
        std::cerr << "ERROR: Signer type value must be 0, 1, 2, 3 or 4"
                  << std::endl;
        return 2;
    }

    info(opts);
    return run(opts);
}
} // namespace xrdndnproducer

int main(int argc, char **argv) { return xrdndnproducer::main(argc, argv); }
