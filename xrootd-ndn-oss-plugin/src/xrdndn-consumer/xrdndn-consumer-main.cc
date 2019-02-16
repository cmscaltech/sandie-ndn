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
#include <stdlib.h>

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "../common/xrdndn-logger.hh"
#include "xrdndn-consumer.hh"

namespace xrdndnconsumer {

uint64_t bufferSize(262144);

int copyFileOverNDN(std::string filePath) {
    Consumer consumer;
    try {
        int retOpen = consumer.Open(filePath);
        if (retOpen) {
            NDN_LOG_ERROR("Unable to open file: " << filePath);
            return -1;
        }

        struct stat info;
        int retFstat = consumer.Fstat(&info, filePath);
        if (retFstat) {
            NDN_LOG_ERROR("Unable to get fstat for file: " << filePath);
            return -1;
        }

        off_t offset = 0;
        std::string buff(bufferSize, '\0');

        int retRead = consumer.Read(&buff[0], offset, bufferSize, filePath);
        while (retRead > 0) {
            offset += retRead;
            retRead = consumer.Read(&buff[0], offset, bufferSize, filePath);
        }

        int retClose = consumer.Close(filePath);
        if (retClose) {
            NDN_LOG_WARN("Unable to close file: " << filePath);
        }
    } catch (const std::exception &e) {
        NDN_LOG_ERROR(e.what());
    }

    return 0;
}

int runTest(std::string dirPath, std::string filePath) {
    int ret = 1;

    auto startCopyFileOverNDN = [&](boost::filesystem::path filePath) {
        try {
            auto absolutePath = boost::filesystem::canonical(
                                    filePath, boost::filesystem::current_path())
                                    .string();
            ret &= copyFileOverNDN(absolutePath);
        } catch (const boost::filesystem::filesystem_error &e) {
            NDN_LOG_ERROR(e.what());
            ret &= 2;
        }
    };

    // TODO: For the moment this will work only for local directories
    // as we don't offer support for directory operations over NDN yet.
    if (!dirPath.empty()) {
        if (boost::filesystem::is_directory(dirPath)) {
            for (auto &entry : boost::make_iterator_range(
                     boost::filesystem::directory_iterator(dirPath), {}))
                startCopyFileOverNDN(entry.path());
        } else {
            NDN_LOG_INFO(
                dirPath
                << " is not a local directory. For the moment this option "
                   "works only for local directories.");
        }
    }

    if (boost::filesystem::exists(boost::filesystem::path(filePath)))
        startCopyFileOverNDN(boost::filesystem::path(filePath));
    else
        ret &= copyFileOverNDN(filePath);

    return ret;
}

static void usage(std::ostream &os, const std::string &programName,
                  const boost::program_options::options_description &desc) {
    os << "Usage: export NDN_LOG=\"xrdndnconsumer*=INFO\" && " << programName
       << " [options]\nYou must specify one of dir/file parameters. \n\n"
       << desc;
}

static void info(uint64_t bufferSz, std::string dirPath, std::string filePath) {
    if (dirPath.empty())
        dirPath = "N/A";

    if (filePath.empty())
        filePath = "N/A";

    NDN_LOG_INFO(
        "\nThe NDN Consumer used in the NDN based filesystem plugin for "
        "XRootD.\nDeveloped by Caltech@CMS.\n\nSelected Options: Read buffer "
        "size: "
        << bufferSz << "B, Path to directory: " << dirPath
        << ", Path to file: " << filePath << "\n");
}

int main(int argc, char **argv) {
    std::string programName = argv[0];

    std::string filePath;
    std::string dirPath;

    boost::program_options::options_description description("Options");
    description.add_options()(
        "bsize,b",
        boost::program_options::value<uint64_t>(&bufferSize)
            ->default_value(bufferSize),
        "Maximum size of the read buffer. The default is 262144. Specify any "
        "value between 8KB and 1GB in bytes.")(
        "dir,d", boost::program_options::value<std::string>(&dirPath),
        "Path to a local directory of files to be copied via NDN.")(
        "file,f", boost::program_options::value<std::string>(&filePath),
        "Path to the file to be copied via NDN.")(
        "help,h", "Print this help message and exit");

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
    }

    if (bufferSize < 8192 || bufferSize > 1073741824) {
        NDN_LOG_ERROR("Buffer size must be between 8KB and 1GB.");
        return 2;
    }

    info(bufferSize, dirPath, filePath);
    return runTest(dirPath, filePath);
}
} // namespace xrdndnconsumer

int main(int argc, char **argv) { return xrdndnconsumer::main(argc, argv); }