// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <fstream>
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
    std::string configFile("");

    boost::program_options::options_description description("Options", 120);
    description.add_options()(
        "config-file", boost::program_options::value<std::string>(&configFile),
        "Configuration file for running xrdndn-producer as a service")(
        "disable-signing",
        boost::program_options::bool_switch(&opts.disableSigning),
        "Eliminate signing among authorized partners by signing Data with a "
        "dummy signature. By default Data is signed using SHA-256")(
        "freshness-period",
        boost::program_options::value<uint64_t>(&opts.freshnessPeriod)
            ->default_value(opts.freshnessPeriod)
            ->implicit_value(opts.freshnessPeriod),
        "Interest packets freshness period in seconds")(
        "garbage-collector-timer",
        boost::program_options::value<uint32_t>(&opts.gbTimePeriod)
            ->default_value(XRDNDN_GB_DEFAULT_TIMEPERIOD)
            ->implicit_value(XRDNDN_GB_MIN_TIMEPERIOD),
        std::string(
            "Periodic timer in seconds that will close files that have reached "
            "their life "
            "time (garbage-collector-lifetime arg). Specify a non-negative "
            "integer larger than " +
            std::to_string(XRDNDN_GB_MIN_TIMEPERIOD))
            .c_str())(
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
            ->default_value(XRDNDN_INTERESTMANAGER_DEFAULT_NTHREADS)
            ->implicit_value(XRDNDN_INTERESTMANAGER_MIN_NTHREADS),
        "Number of threads to handle Interest packets concurrently")(
        "version,V", "Show version information and exit");

    info();

    boost::program_options::variables_map vm;
    try {
        boost::program_options::store(
            boost::program_options::command_line_parser(argc, argv)
                .options(description)
                .run(),
            vm);
        boost::program_options::notify(vm);

        if (vm.count("config-file") > 0) {
            std::ifstream config_file(configFile);
            std::cout << "Reading configuration file: " << configFile
                      << std::endl;
            if (config_file.good()) {
                vm = boost::program_options::variables_map();
                boost::program_options::store(
                    boost::program_options::parse_config_file(config_file,
                                                              description),
                    vm);
                boost::program_options::notify(vm);
            } else {
                std::cerr << "Configuration file: " << configFile
                          << " is not valid. Using default configuration"
                          << std::endl;
            }
        }
    } catch (const boost::program_options::error &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 2;
    } catch (const boost::bad_any_cast &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 2;
    }

    std::string programName = argv[0];

    if (vm.count("help") > 0) {
        usage(std::cout, programName, description);
        return 0;
    }

    if (vm.count("garbage-collector-timer") > 0) {
        if (opts.gbTimePeriod < XRDNDN_GB_MIN_TIMEPERIOD) {
            std::cerr << "ERROR: The minimum time period to trigger the "
                         "garbage collector is "
                      << XRDNDN_GB_MIN_TIMEPERIOD << std::endl;
            return 2;
        }
    }

    opts.gbTimer = std::chrono::seconds(opts.gbTimePeriod);
    opts.freshnessPeriod *= 1000;

    if (vm.count("nthreads") > 0) {
        if (opts.nthreads < XRDNDN_INTERESTMANAGER_MIN_NTHREADS) {
            std::cerr << "ERROR: The minimum number of threads for processing "
                         "Interest packets is "
                      << XRDNDN_INTERESTMANAGER_MIN_NTHREADS << std::endl;
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
