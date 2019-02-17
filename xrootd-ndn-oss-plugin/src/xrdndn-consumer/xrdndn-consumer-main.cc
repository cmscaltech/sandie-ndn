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

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "../common/xrdndn-logger.hh"
#include "xrdndn-consumer.hh"

namespace xrdndnconsumer {

struct Opts {
    std::string filepath;
    std::string dirpath;
    std::string outputpath;
};

struct Opts opts;
uint64_t bufferSize(262144);
static const std::string defaultOutputFileName = std::string("ndnfile.out");

int readFile(std::string filePath, std::string outputPath) {
    auto consumer = Consumer::getXrdNdnConsumerInstance();
    if (!consumer)
        return -1;

    try {
        std::map<uint64_t, std::pair<std::string, uint64_t>> contentStore;

        int retOpen = consumer->Open(filePath);
        if (retOpen != XRDNDN_ESUCCESS) {
            NDN_LOG_ERROR("Unable to open file: " << filePath);
            return -1;
        }

        struct stat info;
        int retFstat = consumer->Fstat(&info, filePath);
        if (retFstat != XRDNDN_ESUCCESS) {
            NDN_LOG_ERROR("Unable to get fstat for file: " << filePath);
            return -1;
        }

        off_t offset = 0;
        std::string buff(bufferSize, '\0');

        int retRead = consumer->Read(&buff[0], offset, bufferSize, filePath);
        while (retRead > 0) {
            contentStore[offset] = std::make_pair(std::string(buff), retRead);
            offset += retRead;
            retRead = consumer->Read(&buff[0], offset, bufferSize, filePath);
        }

        int retClose = consumer->Close(filePath);
        if (retClose != XRDNDN_ESUCCESS) {
            NDN_LOG_WARN("Unable to close file: " << filePath);
        }

        if (!outputPath.empty()) {
            std::ofstream ostream(outputPath, std::ofstream::binary);
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

int run() {
    int ret = 1;

    auto beginReadFile = [&](boost::filesystem::path path) {
        try {
            auto absolutePath = boost::filesystem::canonical(
                                    path, boost::filesystem::current_path())
                                    .string();
            ret &= readFile(absolutePath, opts.outputpath);
        } catch (const boost::filesystem::filesystem_error &e) {
            NDN_LOG_ERROR(e.what());
            ret &= 2;
        }
    };

    // TODO: For the moment this will work only for local directories
    // as we don't offer support for directory operations over NDN yet.
    if (!opts.dirpath.empty()) {
        if (boost::filesystem::is_directory(opts.dirpath)) {
            for (auto &entry : boost::make_iterator_range(
                     boost::filesystem::directory_iterator(opts.dirpath), {}))
                beginReadFile(entry.path());
        } else {
            NDN_LOG_INFO(
                opts.dirpath
                << " is not a local directory. For the moment this option "
                   "works only for local directories.");
        }
    }

    if (!opts.filepath.empty()) {
        ret &= readFile(opts.filepath, opts.outputpath);
    }

    return ret;
}

static void usage(std::ostream &os, const std::string &programName,
                  const boost::program_options::options_description &desc) {
    os << "Usage: export NDN_LOG=\"xrdndnconsumer*=INFO\" && " << programName
       << " [options]\nYou must specify one of dir/file parameters. \n\n"
       << desc;
}

static void info(std::string dir, std::string file, std::string out) {
    if (dir.empty())
        dir = "N/A";

    if (file.empty())
        file = "N/A";

    if (out.empty())
        out = "N/A";

    NDN_LOG_INFO(
        "\nThe NDN Consumer used in the NDN based filesystem plugin for "
        "XRootD.\nDeveloped by Caltech@CMS.\n\nSelected Options: Read buffer "
        "size: "
        << bufferSize << "B, Path to directory: " << dir << ", Path to file: "
        << file << ", Path to output file: " << out << "\n");
}

int main(int argc, char **argv) {
    std::string programName = argv[0];

    boost::program_options::options_description description("Options");
    description.add_options()(
        "bsize,b",
        boost::program_options::value<uint64_t>(&bufferSize)
            ->default_value(bufferSize),
        "Maximum size of the read buffer. The default is 262144. Specify any "
        "value between 8KB and 1GB in bytes.")(
        "dir,d", boost::program_options::value<std::string>(&opts.dirpath),
        "Experimental. Path to a local directory of files to be copied via "
        "NDN.")("file,f",
                boost::program_options::value<std::string>(&opts.filepath),
                "Path to the file to be copied via NDN.")(
        "help,h", "Print this help message and exit.")(
        "output-file,o",
        boost::program_options::value<std::string>(&opts.outputpath),
        "Output path.");

    boost::program_options::variables_map vm;
    try {
        boost::program_options::store(
            boost::program_options::command_line_parser(argc, argv)
                .options(description)
                .run(),
            vm);
        boost::program_options::notify(vm);
    } catch (const boost::program_options::error &e) {
        NDN_LOG_ERROR(e.what());
        return 2;
    } catch (const boost::bad_any_cast &e) {
        NDN_LOG_ERROR(e.what());
        return 2;
    }

    if (vm.count("help") > 0) {
        usage(std::cout, programName, description);
        return 0;
    }

    if ((vm.count("file") == 0) && (vm.count("dir") == 0)) {
        usage(std::cerr, programName, description);
        NDN_LOG_ERROR("Specify path to a file or a directory if files to "
                      "be copied over NDN.");
        return 2;
    } else if (vm.count("file") != 0) {
        if (boost::filesystem::exists(boost::filesystem::path(opts.filepath)))
            opts.filepath =
                boost::filesystem::canonical(opts.filepath,
                                             boost::filesystem::current_path())
                    .string();
    }

    if (bufferSize < 8192 || bufferSize > 1073741824) {
        NDN_LOG_ERROR("Buffer size must be between 8KB and 1GB.");
        return 2;
    }

    if (vm.count("output-file") != 0) {
        if (opts.outputpath.empty()) {
            opts.outputpath =
                boost::filesystem::path(defaultOutputFileName).string();
        } else if (boost::filesystem::is_directory(opts.outputpath)) {
            opts.outputpath = (boost::filesystem::path(opts.outputpath) /
                               boost::filesystem::path(defaultOutputFileName))
                                  .string();
        }
    }

    info(opts.dirpath, opts.filepath, opts.outputpath);
    return run();
}
} // namespace xrdndnconsumer

int main(int argc, char **argv) { return xrdndnconsumer::main(argc, argv); }