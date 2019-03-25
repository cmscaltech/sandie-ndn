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

#include "../common/xrdndn-logger.hh"
#include "xrdndn-producer.hh"

namespace xrdndnproducer {
int run(const Options &opts) {
    boost::asio::io_service ioService;
    ndn::Face face(ioService);

    auto producer = Producer::getXrdNdnProducerInstance(face, opts);
    if (!producer)
        return 2;

    try {
        face.processEvents();
    } catch (const std::exception &e) {
        NDN_LOG_ERROR("[main]: " << e.what());
        return 2;
    }

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
        "XRootD.\nDeveloped by Caltech@CMS.\n\nSelected Options: "
        "Freshness period: "
        << opts.freshnessPeriod
        << "msec, "
           "Garbage collector timer: "
        << opts.gbTimePeriod
        << "sec, "
           "Garbage collector lifetime: "
        << opts.gbFileLifeTime
        << "sec, "
           "Number of threads: "
        << opts.threads
        << ", "
           "Pre-cache files: "
        << opts.precacheFile << "\n");
}

int main(int argc, char **argv) {
    Options opts;

    boost::program_options::options_description description("Options");
    description.add_options()(
        "freshness-period,fp",
        boost::program_options::value<uint64_t>(&opts.freshnessPeriod)
            ->default_value(opts.freshnessPeriod)
            ->implicit_value(opts.freshnessPeriod),
        "Interest packets freshness period in seconds")(
        "garbage-collector-timer",
        boost::program_options::value<uint32_t>(&opts.gbTimePeriod)
            ->default_value(opts.gbTimePeriod)
            ->implicit_value(opts.gbTimePeriod),
        "Time period in seconds when files that have reached their maximum "
        "time without being accessed will be closed and memory will be "
        "cleaned")("garbage-collector-lifetime",
                   boost::program_options::value<int64_t>(&opts.gbFileLifeTime)
                       ->default_value(opts.gbFileLifeTime)
                       ->implicit_value(opts.gbFileLifeTime),
                   "Maximum time in seconds that a file will be left opened "
                   "without being accessed")(
        "help,h", "Print this help message and exit")(
        "threads,t",
        boost::program_options::value<uint16_t>(&opts.threads)
            ->default_value(opts.threads)
            ->implicit_value(opts.threads),
        "Number of threads to handle Interest packets in parallel")(
        "precache-files,P",
        boost::program_options::bool_switch(&opts.precacheFile),
        "Precache files in memory before responding to read interests");

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

    opts.gbTimer = std::chrono::seconds(opts.gbTimePeriod);
    opts.freshnessPeriod *= 1000;

    std::string programName = argv[0];

    if (vm.count("help") > 0) {
        usage(std::cout, programName, description);
        return 0;
    }

    if (vm.count("garbage-collector-lifetime") > 0) {
        if (opts.gbFileLifeTime < 0) {
            std::cerr << "ERROR: Opened file lifetime for garbage collector "
                         "must be a positive number"
                      << std::endl;
            return 2;
        }
    }

    info(opts);
    return run(opts);
}
} // namespace xrdndnproducer

int main(int argc, char **argv) { return xrdndnproducer::main(argc, argv); }
