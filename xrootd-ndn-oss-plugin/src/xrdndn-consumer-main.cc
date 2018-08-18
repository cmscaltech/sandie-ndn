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

#include <iostream>

#include "xrdndn-consumer.hh"

int main(int argc, char **argv) {
    xrdndn::Consumer consumer;
    try {
        consumer.Open(std::string("/root/test/path/for/ndn/xrd/test.txt"));
        std::cout << std::endl;

        consumer.Open(std::string("/system/path/to/inexistent/file"));
        std::cout << std::endl;

        consumer.Close(std::string("/root/test/path/for/ndn/xrd/test.txt"));
        std::cout << std::endl;

        consumer.Close(std::string("/system/path/to/inexistent/file"));
        std::cout << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "ERROR: xrdndnconsumer: " << e.what() << std::endl;
    }
    return 0;
}