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

#include "ft-client-utils.hpp"
#include "ft-client.hpp"

using namespace std;
using namespace indicators;
namespace po = boost::program_options;

static ndnc::Face *face;
static ndnc::benchmark::ft::Runner *client;
static std::vector<std::thread> workers;

void cleanOnExit() {
    if (client != nullptr) {
        client->stop();

        for (auto it = workers.begin(); it != workers.end(); ++it) {
            if (it->joinable()) {
                it->join();
            }
        }

        delete client;
    }

    if (face != nullptr) {
        delete face;
    }
}

void signalHandler(sig_atomic_t signum) {
    cleanOnExit();
    exit(signum);
}

static void usage(ostream &os, const string &app,
                  const po::options_description &desc) {
    os << "Usage: " << app
       << " [options]\nNote: This application needs --file arguments "
          "to be specified\n\n"
       << desc;
}

int main(int argc, char *argv[]) {
    ndnc::benchmark::ft::ClientOptions opts;
    std::string pipelineType = "fixed";

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
    description.add_options()("nthreads",
                              po::value<uint16_t>(&opts.nthreads)
                                  ->default_value(opts.nthreads)
                                  ->implicit_value(opts.nthreads),
                              "The number of worker threads. Half request "
                              "Interest packets and half process Data packets");
    description.add_options()(
        "pipeline-type",
        po::value<string>(&pipelineType)->default_value(pipelineType),
        "Pipeline type. Available options: fixed");
    description.add_options()("pipeline-size",
                              po::value<uint16_t>(&opts.pipelineSize)
                                  ->default_value(opts.pipelineSize),
                              "Maximum pipeline size");
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
            cerr << "ERROR: invalid MTU size. please specify a positive "
                    "integer between 64 and 9000\n\n";
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

    if (vm.count("lifetime") > 0) {
        opts.lifetime = ndn::time::milliseconds(
            vm["lifetime"].as<ndn::time::milliseconds::rep>());
    }

    if (opts.lifetime < ndn::time::milliseconds{0}) {
        cerr << "ERROR: negative lifetime argument value\n\n";
        usage(cout, app, description);
        return 2;
    }

    if (vm.count("file") == 0) {
        cerr << "ERROR: please specify a file URL to be copied over NDN\n\n";
        usage(cerr, app, description);
        return 2;
    }

    if (opts.file.empty()) {
        cerr << "\nERROR: the file URL cannot be an empty string\n";
        return 2;
    }

    if (pipelineType.compare("fixed") == 0) {
        opts.pipelineType = ndnc::PipelineType::fixed;
    } else if (pipelineType.compare("aimd") == 0){
        opts.pipelineType = ndnc::PipelineType::aimd;
    }

    if (opts.pipelineType == ndnc::PipelineType::undefined) {
        cerr << "ERROR: invalid pipeline type\n\n";
        usage(cout, app, description);
        return 2;
    }

    std::cout << "NDNc FILE TRANSFER CLIENT\n";
    signal(SIGINT, signalHandler);

    face = new ndnc::Face();
#ifndef __APPLE__
    if (!face->openMemif(opts.mtu, opts.gqlserver, "ndncft-client"))
        return 2;
#endif

    if (!face->isValid()) {
        cerr << "ERROR: invalid face\n";
        return 2;
    }

    opts.nthreads = opts.nthreads % 2 == 1 ? opts.nthreads + 1 : opts.nthreads;

    client = new ndnc::benchmark::ft::Runner(*face, opts);

    uint64_t totalBytesToTransfer = 0;
    client->getFileInfo(&totalBytesToTransfer);

    if (totalBytesToTransfer == 0) {
        cleanOnExit();
        return -2;
    }

    BlockProgressBar bar{option::BarWidth{80},
                         option::PrefixText{"Downloading "},
                         option::ShowPercentage{true},
                         option::MaxProgress{totalBytesToTransfer}};
    show_console_cursor(true);

    auto start = std::chrono::high_resolution_clock::now();
    std::atomic<uint64_t> bytesToTransfer = 0;

    for (auto wid = 0; wid < opts.nthreads / 2; ++wid) {
        workers.push_back(std::thread(
            &ndnc::benchmark::ft::Runner::requestFileContent, client, wid));
    }

    for (auto wid = 0; wid < opts.nthreads / 2; ++wid) {
        workers.push_back(std::thread(
            &ndnc::benchmark::ft::Runner::receiveFileContent, client,
            [&](uint64_t progress) {
                bytesToTransfer.fetch_add(progress, std::memory_order_release);

                bar.set_progress(bytesToTransfer);
                bar.set_option(
                    option::PostfixText{to_string(bytesToTransfer) + "/" +
                                        to_string(totalBytesToTransfer)});
                bar.tick();
            }));
    }

    for (auto it = workers.begin(); it != workers.end(); ++it) {
        if (it->joinable()) {
            it->join();
        }
    }

    auto duration = std::chrono::high_resolution_clock::now() - start;
    double goodput = ((double)bytesToTransfer * 8.0) /
                     (duration / std::chrono::nanoseconds(1)) * 1e9;

#ifdef DEBUG
    double throughput = ((double)face->readCounters()->nRxBytes * 8.0) /
                        (duration / std::chrono::nanoseconds(1)) * 1e9;
#endif

    cout << "\n--- statistics --\n"
         << client->readCounters()->nInterest << " packets transmitted, "
         << client->readCounters()->nData << " packets received\n"
         << "goodput: " << humanReadableSize(goodput, 'b') << "/s"
#ifdef DEBUG
         << ", throughput: " << humanReadableSize(throughput, 'b') << "/s\n";
#else
         << "\n";
#endif

#ifdef DEBUG
    cout << "\n--- face statistics --\n"
         << face->readCounters()->nTxPackets << " packets transmitted, "
         << face->readCounters()->nRxPackets << " packets received\n"
         << face->readCounters()->nTxBytes << " bytes transmitted, "
         << face->readCounters()->nRxBytes << " bytes received "
         << "with " << face->readCounters()->nErrors << " errors\n";
#endif // DEBUG

    cleanOnExit();
    return 0;
}
