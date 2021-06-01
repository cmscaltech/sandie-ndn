/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2021 California Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <fstream>
#include <iostream>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "../common.hpp"
#include "ping-server.hpp"

using namespace std;
namespace po = boost::program_options;

static bool faceLoop = true;

void handler(sig_atomic_t signal) {
    faceLoop = false;
}

static void usage(ostream &os, const string &app,
                  const po::options_description &desc) {
    os << "Usage: " << app
       << " [options]\nNote: This application needs --prefix argument "
          "specified\n\n"
       << desc;
}

int main(int argc, char **argv) {
    ndnc::ping::server::Options opts;

    po::options_description description("Options", 120);
    description.add_options()(
        "payload-size,s",
        po::value<size_t>(&opts.payloadSize)->default_value(opts.payloadSize),
        string("The payload size expressed in bytes of each NDN Data packet. "
               "Specify a non-negative integer smaller or equal to " +
               to_string(ndn::MAX_NDN_PACKET_SIZE) +
               ". Note that the NDN maximum packet size is " +
               to_string(ndn::MAX_NDN_PACKET_SIZE) +
               " bytes which counts for the Name, Payload and Signing "
               "information.")
            .c_str())(
        "prefix,p", po::value<string>(&opts.prefix),
        "The NDN prefix this application advertises. All packet Names with "
        "this prefix will be processed "
        "by this application.")("help,h", "Print this help message and exit");

    po::variables_map vm;
    try {
        po::store(
            po::command_line_parser(argc, argv).options(description).run(), vm);
        po::notify(vm);
    } catch (const po::error &e) {
        cerr << "ERROR: " << e.what() << "\n";
        return 2;
    } catch (const boost::bad_any_cast &e) {
        cerr << "ERROR: " << e.what() << "\n";
        return 2;
    }

    string app = argv[0];

    if (vm.count("help") > 0) {
        usage(cout, app, description);
        return 0;
    }

    if (vm.count("prefix") == 0) {
        usage(cerr, app, description);
        cerr << "\nERROR: Please specify the NDN prefix this applications "
                "advertises\n";
        return 2;
    }

    if (vm.count("payload-size") > 0) {
        if (opts.payloadSize < 0 ||
            opts.payloadSize > ndn::MAX_NDN_PACKET_SIZE) {
            usage(cout, app, description);
            cerr << "\nERROR: Invalid payload size. Please specify a "
                    "non-negative integer smaller or eqaul to "
                 << ndn::MAX_NDN_PACKET_SIZE << "\n";
            return 2;
        }
    }

    std::cout << "TRACE: Starting NDNc Ping Server...\n";

    signal(SIGINT, handler);
    signal(SIGABRT, handler);

    ndnc::Face *face = new ndnc::Face();
    if (!face->isValid()) {
        cerr << "ERROR: Could not create face\n";
    }

    ndnc::ping::server::Runner *server =
        new ndnc::ping::server::Runner(*face, opts);
    face->advertise(opts.prefix);

    while (faceLoop && face->isValid()) {
        face->loop();
    }

#ifdef DEBUG
    cout << "face counters: ";
    face->readCounters();
#endif // DEBUG

    cout << "\n"
         << server->readCounters().nTxData << " packets transmitted, "
         << server->readCounters().nRxInterests << " packets received\n";

    if (NULL != server) {
        delete server;
    }

    if (NULL != face) {
        delete face;
    }

    return 0;
}
