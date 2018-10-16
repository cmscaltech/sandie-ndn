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
#include <thread>
#include <vector>

#include "xrdndn-consumer.hh"

static const char *filesPath = "/root/test/path/for/ndn/xrd/";
static const char *ext = ".out";
static const std::vector<const char *> filesList = {
    "inexistent_file",
    "test.txt",
    "convict-lake-autumn-4k-2k.jpg",
    "wallpaper.wiki-Uhd-8k-river-wallpaperr-PIC-WPE0013289.jpg"//,
    /*"empty_file.dat",
    "1gb_file",
    "2gb_file"*/};

int _test_readFile(std::string path, std::string outputPath) {
    xrdndnconsumer::Consumer consumer;

    try {
        std::ofstream output(outputPath, std::ofstream::binary);

        int retOpen = consumer.Open(path);
        if (retOpen)
            return -1;

        struct stat info;
        int retFstat = consumer.Fstat(&info, path);

        if (retFstat == 0) {
            std::cout << "file_size = " << info.st_size << std::endl;
        } else {
            return -1;
        }

        size_t blen = 262144; // 32768;
        off_t offset = 0;
        std::string buff(blen, '\0');

        int ret = consumer.Read(&buff[0], offset, blen, path);
        while (ret > 0) {
            output.write(buff.data(), ret);
            offset += ret;
            ret = consumer.Read(&buff[0], offset, blen, path);
        }
        output.close();
        consumer.Close(path);
    } catch (const std::exception &e) {
        std::cerr << "ERROR: _test_readFile: " << e.what() << std::endl;
    }

    return 0;
}

int main(int, char **) {
    // Single thread tests
    for (uint64_t i = 0; i < filesList.size(); ++i) {
        std::string pathIn(filesPath);
        pathIn += filesList[i];

        std::string pathOut = pathIn + ext;
        std::cout << "Read file: " << pathIn << " to: " << pathOut << std::endl;

        _test_readFile(pathIn, pathOut);
    }

    // Multi-threading tests
    // std::thread threads[filesList.size()];

    // for (unsigned i = 0; i < filesList.size(); ++i) {
    //     std::string pathIn(filesPath);
    //     pathIn += filesList[i];

    //     std::string pathOut = pathIn + ext;
    //     std::cout << "Thread [" << i << "] Read file: " << pathIn
    //               << " to: " << pathOut << std::endl;

    //     threads[i] = std::thread(_test_readFile, pathIn, pathOut);
    // }

    // for (unsigned i = 0; i < filesList.size(); ++i) {
    //     threads[i].join();
    // }

    return 0;
}