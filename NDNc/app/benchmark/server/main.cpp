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
#include "logger/logger.hpp"

using namespace std;
namespace po = boost::program_options;

static ndnc::face::Face *face;
static ndnc::benchmark::ft::Runner *server;
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
        "The GraphQL server address");
    description.add_options()(
        "mtu", po::value<size_t>(&opts.mtu)->default_value(opts.mtu),
        "Dataroom size. Specify a positive integer between 64 and 9000");
    description.add_options()(
        "name-prefix",
        po::value<string>(&opts.namePrefix)->default_value(opts.namePrefix),
        "The NDN Name prefix this producer application advertises. Specify a "
        "non-empty string");
    description.add_options()(
        "segment-size",
        po::value<size_t>(&opts.segmentSize)->default_value(opts.segmentSize),
        string("The maximum segment size of each Data packet that has "
               "content from a file. Specify a positive integer smaller or "
               "equal to " +
               to_string(ndn::MAX_NDN_PACKET_SIZE))
            .c_str());
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
            cerr << "ERROR: invalid MTU size\n\n";
            usage(cout, description);
            return 2;
        }
    }

    if (vm.count("gqlserver") > 0) {
        if (opts.gqlserver.empty()) {
            cerr << "ERROR: empty gqlserver argument value\n\n";
            usage(cout, description);
            return 2;
        }
    }

    if (vm.count("segment-size") > 0) {
        if (opts.segmentSize > ndn::MAX_NDN_PACKET_SIZE) {
            cerr << "ERROR: invalid segment size value\n\n";
            usage(cout, description);
            return 2;
        }
    }

    if (vm.count("name-prefix") > 0) {
        if (opts.namePrefix.empty()) {
            cerr << "ERROR: empty name prefix value\n\n";
            usage(cout, description);
            return 2;
        }
    }

    opts.namePrefixNoComponents = ndn::Name(opts.namePrefix).size();

    face = new ndnc::face::Face();
    if (!face->connect(opts.mtu, opts.gqlserver, "ndncft-server")) {
        return 2;
    }

    server = new ndnc::benchmark::ft::Runner(*face, opts);

    if (!face->advertise(opts.namePrefix)) {
        cerr << "ERROR: unable to advertise prefix on face\n";
        return 2;
    }

    LOG_INFO("running... ");

    while (shouldRun && face->isConnected()) {
        face->loop();
    }

    cout << endl;

    if (server != nullptr) {
        delete server;
    }

    if (face != nullptr) {
        delete face;
    }

    return 0;
}
