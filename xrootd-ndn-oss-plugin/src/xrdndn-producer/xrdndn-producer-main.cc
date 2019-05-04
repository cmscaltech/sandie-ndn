/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018-2019 California Institute of Technology                   *
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

#include <ndn-cxx/version.hpp>

#include <boost/config.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/version.hpp>

#include "../common/xrdndn-logger.hh"
#include "xrdndn-producer-version.hh"
#include "xrdndn-producer.hh"

namespace xrdndnproducer {
int run(const Options &opts) {
    boost::asio::io_service ioService;
    ndn::Face face(ioService);

    auto producer = Producer::getXrdNdnProducerInstance(face, opts);
    if (!producer) {
        return 2;
    }

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
    os << "Usage: " << programName
       << " [options]\nNote: This application can run without arguments\n\n"
       << desc;
}

static void info() {
    std::cout
        << "The suitable NDN Producer for the NDN based filesystem plugin for "
           "XRootD.\nDeveloped by Caltech@CMS\n"
        << std::endl;
}

int main(int argc, char **argv) {
    Options opts;
    std::string logLevel("INFO");

    boost::program_options::options_description description("Options", 120);
    description.add_options()(
        "disable-signing",
        boost::program_options::bool_switch(&opts.disableSigning),
        "Eliminate signing among authorized partners. By default Data is "
        "singed using SHA-256. By using this argument, the data will be signed "
        "with a fake signature, thus increasing the performance but also the "
        "risk of data being corrupted")(
        "freshness-period",
        boost::program_options::value<uint64_t>(&opts.freshnessPeriod)
            ->default_value(opts.freshnessPeriod)
            ->implicit_value(opts.freshnessPeriod),
        "Interest packets freshness period in seconds")(
        "garbage-collector-timer",
        boost::program_options::value<uint32_t>(&opts.gbTimePeriod)
            ->default_value(opts.gbTimePeriod)
            ->implicit_value(opts.gbTimePeriod),
        "Recurrent time in seconds when files that have reached their "
        "life time without being accessed will be closed")(
        "garbage-collector-lifetime",
        boost::program_options::value<int64_t>(&opts.gbFileLifeTime)
            ->default_value(opts.gbFileLifeTime)
            ->implicit_value(opts.gbFileLifeTime),
        "Life time in seconds that a file will be left open without being "
        "accessed. Once the limit is reached and garbage-collector-timer "
        "triggers, the file will be closed")(
        "help,h", "Print this help message and exit")(
        "log-level",
        boost::program_options::value<std::string>(&logLevel)
            ->default_value(logLevel)
            ->implicit_value("NONE"),
        "Log level: TRACE, DEBUG, INFO, WARN, ERROR, FATAL. More information "
        "can be found at "
        "https://named-data.net/doc/ndn-cxx/current/manpages/ndn-log.html")(
        "nthreads",
        boost::program_options::value<uint16_t>(&opts.nthreads)
            ->default_value(opts.nthreads)
            ->implicit_value(opts.nthreads),
        "Number of threads to handle Interest packets concurrently. Must be at "
        "least 1")("version,V", "Show version information and exit");

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

    if (vm.count("nthreads") > 0) {
        if (opts.nthreads < 1) {
            std::cerr << "ERROR: Number of threads must be at least 1"
                      << std::endl;
            return 2;
        }
    }

    if (vm.count("version") > 0) {
        std::cout << XRDNDN_PRODUCER_VERSION_STRING << std::endl;
        return 0;
    }

    try {
        ndn::util::Logging::setLevel(PRODUCER_LOGGER_PREFIX "=" + logLevel);
    } catch (const std::invalid_argument &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        usage(std::cerr, programName, description);
        return 2;
    }

    info();

    NDN_LOG_INFO("XRootD NDN Producer version " XRDNDN_PRODUCER_VERSION_STRING
                 " starting");
    {
        const std::string boostBuildInfo =
            "Boost version " + std::to_string(BOOST_VERSION / 100000) + "." +
            std::to_string(BOOST_VERSION / 100 % 1000) + "." +
            std::to_string(BOOST_VERSION % 100);

        const std::string ndnCxxInfo =
            "ndn-cxx version " NDN_CXX_VERSION_STRING;

        NDN_LOG_INFO(
            "xrdndn-producer build " XRDNDN_PRODUCER_VERSION_BUILD_STRING
            " built with " BOOST_COMPILER ", with " BOOST_STDLIB ", with "
            << boostBuildInfo << ", with " << ndnCxxInfo);
        NDN_LOG_INFO("Selected Options: Freshness period: "
                     << opts.freshnessPeriod
                     << "msec, Garbage collector timer: " << opts.gbTimePeriod
                     << "sec, Garbage collector lifetime: "
                     << opts.gbFileLifeTime
                     << "sec, Number of threads: " << opts.nthreads
                     << ", SHA-256 signing disabled: " << opts.disableSigning);
    }

    return run(opts);
}
} // namespace xrdndnproducer

int main(int argc, char **argv) { return xrdndnproducer::main(argc, argv); }
