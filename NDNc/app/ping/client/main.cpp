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
#include "ping-client.hpp"

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

int main(int argc, char *argv[]) {
    ndnc::ping::client::Options opts;

    po::options_description description("Options", 120);
    description.add_options()(
        "interval,i",
        po::value<ndn::time::milliseconds::rep>()->default_value(100),
        "Interval between Interests in milliseconds. Specify a non-negative "
        "value")("lifetime,l",
                 po::value<ndn::time::milliseconds::rep>()->default_value(1000),
                 "Interest lifetime in milliseconds. Specify a non-negative "
                 "value")("prefix,p", po::value<string>(&opts.prefix),
                          "The NDN prefix this application "
                          "advertises. All packet Names with this "
                          "prefix will be processed by this "
                          "application.")("help,h", "Print this help message "
                                                    "and exit");

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
        cerr << "\nERROR: Please specify the NDN prefix of Interest packets "
                "expressed by this application\n";
        return 2;
    }

    if (vm.count("interval") > 0) {
        opts.interval = ndn::time::milliseconds(
            vm["interval"].as<ndn::time::milliseconds::rep>());
    }

    if (opts.interval < ndn::time::milliseconds{0}) {
        cerr << "\nERROR: Interval between Interests cannot be negative\n";
        usage(cout, app, description);
        return 2;
    }

    if (vm.count("lifetime") > 0) {
        opts.lifetime = ndn::time::milliseconds(
            vm["lifetime"].as<ndn::time::milliseconds::rep>());
    }

    if (opts.lifetime < ndn::time::milliseconds{0}) {
        cerr << "\nERROR: Interests lifetime cannot be negative\n";
        usage(cout, app, description);
        return 2;
    }

    std::cout << "TRACE: Starting NDNc Ping Client...\n";

    signal(SIGINT, handler);
    signal(SIGABRT, handler);

    ndnc::Face *face = new ndnc::Face();
    if (!face->isValid()) {
        cerr << "ERROR: Could not create face\n";
        return -1;
    }

    ndnc::ping::client::Runner *client =
        new ndnc::ping::client::Runner(*face, opts);

    while (faceLoop && face->isValid()) {
        face->loop();
    }

    auto lossRation = (1.0 - ((double)client->readCounters().nRxData /
                              (double)client->readCounters().nTxInterests)) *
                      100;

#ifdef DEBUG
    cout << "face counters: ";
    face->printCounters();
#endif // DEBUG

    cout << "\n"
         << client->readCounters().nTxInterests << " packets transmitted, "
         << client->readCounters().nRxData << " packets received, "
         << lossRation << "% packet loss\n";

    if (NULL != client) {
        delete client;
    }

    if (NULL != face) {
        delete face;
    }

    return 0;
}
