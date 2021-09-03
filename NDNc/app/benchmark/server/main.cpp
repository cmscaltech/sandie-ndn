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
#include <signal.h>
#include <unistd.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "ft-server.hpp"

using namespace std;
namespace po = boost::program_options;

static bool shouldRun = true;

void handler(sig_atomic_t) {
    shouldRun = false;
}

static void usage(ostream &os, const po::options_description &desc) {
    os << desc;
}

int main(int argc, char **argv) {
    signal(SIGINT, handler);
    signal(SIGABRT, handler);

    ndnc::benchmark::ft::ServerOptions opts;

    po::options_description description("Options", 120);
    description.add_options()(
        "gqlserver",
        po::value<string>(&opts.gqlserver)->default_value(opts.gqlserver),
        "GraphQL server address");
    description.add_options()(
        "mtu", po::value<size_t>(&opts.mtu)->default_value(opts.mtu),
        "Dataroom size. Specify a positive integer between 64 and 9000");
    description.add_options()("help,h", "Print this help message and exit");

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

    if (vm.count("help") > 0) {
        usage(cout, description);
        return 0;
    }

    if (vm.count("mtu") > 0) {
        if (opts.mtu < 64 || opts.mtu > 9000) {
            cerr << "ERROR: Invalid MTU size. Please specify a positive "
                    "integer between 64 and 9000\n\n";
            usage(cout, description);
            return 2;
        }
    }

    if (vm.count("gqlserver") > 0) {
        if (opts.gqlserver.empty()) {
            cerr << "ERROR: Empty gqlserver argument value\n\n";
            usage(cout, description);
            return 2;
        }
    }

    std::cout << "NDNc FILE-TRANSFER BENCHMARKING SERVER APP\n";

    ndnc::Face *face = new ndnc::Face();
#ifndef __APPLE__
    if (!face->openMemif(opts.mtu, opts.gqlserver, "ndncft-server"))
        return 2;
#endif
    if (!face->isValid()) {
        cerr << "ERROR: Invalid face\n";
        return 2;
    }

    ndnc::benchmark::ft::Runner *server =
        new ndnc::benchmark::ft::Runner(*face, opts);

    face->advertise(ndnc::benchmark::namePrefixUri);

    while (shouldRun && face->isValid()) {
        face->loop();
    }

#ifdef DEBUG
    cout << "\n--- face statistics --\n"
         << face->readCounters().nTxPackets << " packets transmitted, "
         << face->readCounters().nRxPackets << " packets received\n"
         << face->readCounters().nTxBytes << " bytes transmitted, "
         << face->readCounters().nRxBytes << " bytes received "
         << "with " << face->readCounters().nErrors << " errors\n";
#endif // DEBUG

    if (server != nullptr) {
        delete server;
    }

    if (face != nullptr) {
        delete face;
    }

    return 0;
}
