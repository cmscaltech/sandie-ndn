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

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <unistd.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "ft-client.hpp"
#include "indicators.hpp"

using namespace std;
using namespace indicators;
namespace po = boost::program_options;

static ndnc::benchmark::ft::Runner *client;

void handler(sig_atomic_t) {
    if (client != nullptr) {
        client->stop();
    }
}

static std::string humanReadableSize(double bytes, char suffix = 'B') {
    static char output[1024];
    for (auto unit : {"", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi"}) {
        if (abs(bytes) < 1024.0) {
            sprintf(output, "%3.1f %s%c", bytes, unit, suffix);
            return std::string(output);
        }
        bytes /= 1024.0;
    }

    sprintf(output, "%.1f Yi%c", bytes, suffix);
    return std::string(output);
}

static void usage(ostream &os, const string &app,
                  const po::options_description &desc) {
    os << "Usage: " << app
       << " [options]\nNote: This application needs --file arguments "
          "to be specified\n\n"
       << desc;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handler);
    signal(SIGABRT, handler);

    ndnc::benchmark::ft::ClientOptions opts;

    po::options_description description("Options", 120);
    description.add_options()("file", po::value<string>(&opts.file),
                              "The file path");
    description.add_options()(
        "gqlserver",
        po::value<string>(&opts.gqlserver)->default_value(opts.gqlserver),
        "GraphQL server address");
    description.add_options()(
        "lifetime",
        po::value<ndn::time::milliseconds::rep>()->default_value(
            opts.lifetime.count()),
        "Interest lifetime in milliseconds. Specify a positive integer");
    description.add_options()(
        "mtu", po::value<size_t>(&opts.mtu)->default_value(opts.mtu),
        "Dataroom size. Specify a positive integer between 64 and 9000");
    description.add_options()(
        "nthreads",
        po::value<uint16_t>(&opts.nthreads)
            ->default_value(opts.nthreads)
            ->implicit_value(opts.nthreads),
        "The number of threads/workers to request Data in parallel");
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
            cerr << "ERROR: Invalid MTU size. Please specify a positive "
                    "integer between 64 and 9000\n\n";
            usage(cout, app, description);
            return 2;
        }
    }

    if (vm.count("gqlserver") > 0) {
        if (opts.gqlserver.empty()) {
            cerr << "ERROR: Empty gqlserver argument value\n\n";
            usage(cout, app, description);
            return 2;
        }
    }

    if (vm.count("lifetime") > 0) {
        opts.lifetime = ndn::time::milliseconds(
            vm["lifetime"].as<ndn::time::milliseconds::rep>());
    }

    if (opts.lifetime < ndn::time::milliseconds{0}) {
        cerr << "ERROR: Negative lifetime argument value\n\n";
        usage(cout, app, description);
        return 2;
    }

    if (vm.count("file") == 0) {
        cerr << "ERROR: Please specify a file URL to be copied over NDN\n\n";
        usage(cerr, app, description);
        return 2;
    }

    if (opts.file.empty()) {
        cerr << "\nERROR: The file URL cannot be an empty string\n";
        return 2;
    }

    std::cout << "NDNc FILE-TRANSFER BENCHMARKING CLIENT\n";

    ndnc::Face *face = new ndnc::Face();
#ifndef __APPLE__
    if (!face->openMemif(opts.mtu, opts.gqlserver, "ndncft-client"))
        return 2;
#endif
    if (!face->isValid()) {
        cerr << "ERROR: Invalid face\n";
        return 2;
    }

    client = new ndnc::benchmark::ft::Runner(*face, opts);

    auto fileSize = client->getFileMetadata();
    if (fileSize == 0) {
        client->stop();
        return -2;
    }

    BlockProgressBar bar{
        option::BarWidth{80}, option::PrefixText{"Downloading "},
        option::ShowPercentage{true}, option::MaxProgress{fileSize}};
    show_console_cursor(true);

    auto start = std::chrono::high_resolution_clock::now();
    std::atomic<uint64_t> totalProgress = 0;

    client->run([&](uint64_t progress) {
        totalProgress.fetch_add(progress, std::memory_order_release);
        bar.set_progress(totalProgress);
        bar.set_option(option::PostfixText{to_string(totalProgress) + "/" +
                                           to_string(fileSize)});
        bar.tick();
    });

    client->wait();

    auto duration = std::chrono::high_resolution_clock::now() - start;
    double goodput = ((double)totalProgress * 8.0) /
                     (duration / std::chrono::nanoseconds(1)) * 1000000000;

#ifdef DEBUG
    double throughput = ((double)face->readCounters().nRxBytes * 8.0) /
                        (duration / std::chrono::nanoseconds(1)) * 1000000000;
#endif

    cout << "\n--- statistics --\n"
         << client->readCounters().nInterest << " packets transmitted, "
         << client->readCounters().nData << " packets received\n"
         << "goodput: " << humanReadableSize(goodput, 'b') << "/s"
#ifdef DEBUG
         << ", throughput: " << humanReadableSize(throughput, 'b') << "/s\n";
#else
         << "\n";
#endif

#ifdef DEBUG
    cout << "\n--- face statistics --\n"
         << face->readCounters().nTxPackets << " packets transmitted, "
         << face->readCounters().nRxPackets << " packets received\n"
         << face->readCounters().nTxBytes << " bytes transmitted, "
         << face->readCounters().nRxBytes << " bytes received "
         << "with " << face->readCounters().nErrors << " errors\n";
#endif // DEBUG

    if (client != nullptr) {
        delete client;
    }

    if (face != nullptr) {
        delete face;
    }

    return 0;
}
