/**************************************************************************
 * Named Data Networking plugin for xrootd                                *
 * Copyright © 2018 California Institute of Technology                    *
 *                                                                        *
 * Author: Catalin Iordache <catalin.iordache@cern.ch>                    *
 *                                                                        *
 * This program is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>. *
 **************************************************************************/

#include <fstream>
#include <iostream>

#include "xrdndn-consumer.hh"

int _test_readFile(std::string path, std::string outputPath) {
    xrdndn::Consumer consumer;

    try {
        std::ofstream output(outputPath, std::ofstream::binary);

        int retOpen = consumer.Open(path);
        std::cout << "CONSUMER_MAIN: Open " << path
                  << " error code: " << retOpen << std::endl
                  << std::endl;
        if (retOpen)
            return -1;

        size_t blen = 32768;
        off_t offset = 0;
        std::string buff(blen, '\0');

        int ret = consumer.Read(&buff[0], offset, blen, path);
        while (ret > 0) {
            output.write(buff.data(), ret);
            offset += ret;
            ret = consumer.Read(&buff[0], offset, blen, path);
        }
        output.close();

        int retClose = consumer.Close(path);
        std::cout << "CONSUMER_MAIN: Close " << path
                  << " error code: " << retClose << std::endl
                  << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "ERROR: _test_readFile: " << e.what() << std::endl;
    }

    return 0;
}

int main(int argc, char **argv) {
    _test_readFile(std::string("/root/test/path/for/ndn/xrd/test.txt"),
                   std::string("/root/test/path/for/ndn/xrd/test.txt.out"));

    _test_readFile(
        std::string(
            "/root/test/path/for/ndn/xrd/convict-lake-autumn-4k-2k.jpg"),
        std::string("/root/test/path/for/ndn/xrd/res.jpg"));

    return 0;
}