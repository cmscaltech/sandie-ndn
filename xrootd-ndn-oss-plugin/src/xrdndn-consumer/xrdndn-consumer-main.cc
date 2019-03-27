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

#include <fstream>
#include <iostream>
#include <stdlib.h>

#include <ndn-cxx/version.hpp>

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/version.hpp>

#include "../common/xrdndn-logger.hh"
#include "xrdndn-consumer-version.hh"
#include "xrdndn-consumer.hh"

namespace xrdndnconsumer {

struct Opts {
    std::string infile;
    std::string outfile;
};

struct Opts opts;
uint64_t bufferSize(262144);

int readFile() {
    auto consumer = Consumer::getXrdNdnConsumerInstance();
    if (!consumer) {
        return 2;
    }

    try {
        std::map<uint64_t, std::pair<std::string, uint64_t>> contentStore;

        int retOpen = consumer->Open(opts.infile);
        if (retOpen != XRDNDN_ESUCCESS) {
            NDN_LOG_ERROR("Unable to open file: " << opts.infile << ". "
                                                  << strerror(abs(retOpen)));
            return 2;
        }

        struct stat info;
        int retFstat = consumer->Fstat(&info, opts.infile);
        if (retFstat != XRDNDN_ESUCCESS) {
            NDN_LOG_ERROR("Unable to get fstat for file: "
                          << opts.infile << ". " << strerror(abs(retFstat)));
            return 2;
        }

        off_t offset = 0;
        std::string buff(bufferSize, '\0');

        int retRead = consumer->Read(&buff[0], offset, bufferSize, opts.infile);
        while (retRead > 0) {
            contentStore[offset] = std::make_pair(std::string(buff), retRead);
            offset += retRead;
            retRead = consumer->Read(&buff[0], offset, bufferSize, opts.infile);
        }

        if (retRead < 0)
            NDN_LOG_ERROR("Unable to read file: " << opts.infile << ". "
                                                  << strerror(abs(retRead)));

        int retClose = consumer->Close(opts.infile);
        if (retClose != XRDNDN_ESUCCESS) {
            NDN_LOG_WARN("Unable to close file: " << opts.infile << ". "
                                                  << strerror(abs(retClose)));
        }

        if (!opts.outfile.empty()) {
            std::ofstream ostream(opts.outfile, std::ofstream::binary);
            for (auto it = contentStore.begin(); it != contentStore.end();
                 it = contentStore.erase(it)) {
                ostream.write(it->second.first.data(), it->second.second);
            }
            ostream.close();
        }

        contentStore.clear();
    } catch (const std::exception &e) {
        NDN_LOG_ERROR(e.what());
    }

    return 0;
}

int run() { return readFile(); }

static void usage(std::ostream &os, const std::string &programName,
                  const boost::program_options::options_description &desc) {
    os << "Usage: " << programName
       << " [options]\nNote: This application needs --input-file argument "
          "specified. \n\n"
       << desc;
}

static void info() {
    NDN_LOG_INFO(
        "\nThe NDN Consumer used in the NDN based filesystem plugin for "
        "XRootD.\nDeveloped by Caltech@CMS.\n");
}

int main(int argc, char **argv) {
    std::string programName = argv[0];
    std::string logLevel("INFO");

    boost::program_options::options_description description("Options", 120);
    description.add_options()(
        "bsize",
        boost::program_options::value<uint64_t>(&bufferSize)
            ->default_value(bufferSize)
            ->implicit_value(bufferSize),
        "Read buffer in bytes. Specify any value between 8KB and 1GB in bytes")(
        "help,h", "Print this help message and exit")(
        "input-file", boost::program_options::value<std::string>(&opts.infile),
        "Path to file to be copied over Name Data Networking")(
        "log-level",
        boost::program_options::value<std::string>(&logLevel)
            ->default_value(logLevel)
            ->implicit_value("NONE"),
        "Log level. Available options: TRACE, DEBUG, INFO, WARN, ERROR, "
        "FATAL. More information can be found at "
        "https://named-data.net/doc/ndn-cxx/current/manpages/ndn-log.html")(
        "output-file",
        boost::program_options::value<std::string>(&opts.outfile)
            ->default_value("")
            ->implicit_value("./ndnfile.out"),
        "Path to output file copied over Name Data Networking")(
        "version,V", "Show version information and exit");

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

    if (vm.count("version") > 0) {
        std::cout << XRDNDN_CONSUMER_VERSION_STRING << std::endl;
        return 0;
    }

    if (vm.count("bsize") > 0) {
        if (bufferSize < 1024 || bufferSize > 1073741824) {
            std::cerr << "Buffer size must be between 8KB and 1GB."
                      << std::endl;
            return 2;
        }
    }

    if (vm.count("input-file") == 0) {
        std::cerr << "ERROR: Specify file to be copied over NDN" << std::endl;
        usage(std::cerr, programName, description);
        return 2;
    }

    if (boost::filesystem::exists(boost::filesystem::path(opts.infile))) {
        opts.infile = boost::filesystem::canonical(
                          opts.infile, boost::filesystem::current_path())
                          .string();
    }

    try {
        ndn::util::Logging::setLevel(CONSUMER_LOGGER_PREFIX "=" + logLevel);
    } catch (const std::invalid_argument &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        usage(std::cerr, programName, description);
        return 2;
    }

    if (!opts.outfile.empty())
        opts.outfile = boost::filesystem::path(opts.outfile).string();

    info();

    NDN_LOG_INFO("XRootD NDN Consumer version " XRDNDN_CONSUMER_VERSION_STRING
                 " starting");
    {
        const std::string boostBuildInfo =
            "Boost version " + std::to_string(BOOST_VERSION / 100000) + "." +
            std::to_string(BOOST_VERSION / 100 % 1000) + "." +
            std::to_string(BOOST_VERSION % 100);

        const std::string ndnCxxInfo =
            "ndn-cxx version " NDN_CXX_VERSION_STRING;

        NDN_LOG_INFO(
            "xrdndn-consumer build " XRDNDN_CONSUMER_VERSION_BUILD_STRING
            " built with " BOOST_COMPILER ", with " BOOST_STDLIB ", with "
            << boostBuildInfo << ", with " << ndnCxxInfo);

        NDN_LOG_INFO("Selected Options: Read buffer size: "
                     << bufferSize << "B, Input file: " << opts.infile
                     << ", Output file: "
                     << (opts.outfile.empty() ? "N/D" : opts.outfile));
    }

    return run();
}
} // namespace xrdndnconsumer

int main(int argc, char **argv) { return xrdndnconsumer::main(argc, argv); }