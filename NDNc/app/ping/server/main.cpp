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

static bool shouldRun = true;

void handler(sig_atomic_t) {
    shouldRun = false;
}

static void usage(ostream &os, const string &app,
                  const po::options_description &desc) {
    os << "Usage: " << app
       << " [options]\nNote: This application needs --name argument "
          "to be specified\n\n"
       << desc;
}

int main(int argc, char **argv) {
    signal(SIGINT, handler);
    signal(SIGABRT, handler);

    ndnc::ping::server::Options opts;
    po::options_description description("Options", 120);
    description.add_options()(
        "gqlserver",
        po::value<string>(&opts.gqlserver)->default_value(opts.gqlserver),
        "The GraphQL server address");
    description.add_options()(
        "mtu", po::value<size_t>(&opts.mtu)->default_value(opts.mtu),
        "Dataroom size. Specify a positive integer between 64 and 9000");
    description.add_options()(
        "name", po::value<string>(&opts.name),
        "The NDN Name prefix that this application advertises");
    description.add_options()(
        "payload",
        po::value<size_t>(&opts.payloadLength)
            ->default_value(opts.payloadLength),
        string("The payload length of each NDN Data packet, expressed in "
               "bytes. Specify a positive integer smaller or equal to " +
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

    const string app = argv[0];

    if (vm.count("help") > 0) {
        usage(cout, app, description);
        return 0;
    }

    if (vm.count("mtu") > 0) {
        if (opts.mtu < 64 || opts.mtu > 9000) {
            cerr << "ERROR: invalid MTU size\n\n";
            usage(cout, app, description);
            return 2;
        }
    }

    if (vm.count("gqlserver") > 0) {
        if (opts.gqlserver.empty()) {
            cerr << "ERROR: empty gqlserver argument value\n\n";
            usage(cout, app, description);
            return 2;
        }
    }

    if (vm.count("name") == 0) {
        cerr << "ERROR: no NDN Name prefix specified\n\n";
        usage(cerr, app, description);
        return 2;
    }

    if (vm.count("payload") > 0) {
        if (opts.payloadLength > ndn::MAX_NDN_PACKET_SIZE) {
            cerr << "ERROR: invalid payload length\n\n ";
            usage(cout, app, description);
            return 2;
        }
    }

    auto face = new ndnc::face::Face();
    if (!face->connect(opts.mtu, opts.gqlserver, "ndncping-server")) {
        return 2;
    }

    ndnc::ping::server::Runner *server =
        new ndnc::ping::server::Runner(*face, opts);

    LOG_INFO("running...");

    face->advertise(opts.name);

    while (shouldRun && face->isConnected()) {
        face->loop();
    }

    cout << "\n--- statistics --\n"
         << server->readCounters().nTxData << " packets transmitted, "
         << server->readCounters().nRxInterests << " packets received\n\n";

    if (server != nullptr) {
        delete server;
    }

    if (face != nullptr) {
        delete face;
    }

    return 0;
}
