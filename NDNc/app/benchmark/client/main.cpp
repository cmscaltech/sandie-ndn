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

#include <boost/algorithm/string.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "ft-client-utils.hpp"
#include "ft-client.hpp"
#include "influxdb_upload.hpp"
#include "logger/logger.hpp"

using namespace std;
using namespace indicators;
namespace po = boost::program_options;
namespace al = boost::algorithm;

static ndnc::Face *face;
static ndnc::benchmark::ft::Runner *client;
static ndnc::InfluxDBClient *influxDBClient;
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

    if (influxDBClient != nullptr) {
        delete influxDBClient;
    }
}

void handler(sig_atomic_t signum) {
    cleanOnExit();
    exit(signum);
}

static void usage(ostream &os, const string &app,
                  const po::options_description &desc) {
    os << "Usage: " << app
       << " [options]\nNote: This application needs --file argument "
          "specified\n\n"
       << desc;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handler);

    ndnc::benchmark::ft::ClientOptions opts;
    std::string pipelineType = "fixed";

    po::options_description description("Options", 120);
    description.add_options()("file", po::value<string>(&opts.file),
                              "The path to the file to be copied over NDN");
    description.add_options()(
        "gqlserver",
        po::value<string>(&opts.gqlserver)->default_value(opts.gqlserver),
        "The GraphQL server address");
    // description.add_options()(
    //     "influxdb-addr",
    //     po::value<string>(&opts.influxdbaddr)->default_value(opts.influxdbaddr),
    //     "InfluxDB server address");
    // description.add_options()(
    //     "influxdb-name",
    //     po::value<string>(&opts.influxdbname)->default_value(opts.influxdbname),
    //     "InfluxDB name");
    description.add_options()(
        "lifetime",
        po::value<ndn::time::milliseconds::rep>()->default_value(
            opts.lifetime.count()),
        "The Interest lifetime in milliseconds. Specify a positive integer");
    description.add_options()(
        "mtu", po::value<size_t>(&opts.mtu)->default_value(opts.mtu),
        "Dataroom size. Specify a positive integer between 64 and 9000");
    description.add_options()(
        "nthreads",
        po::value<uint16_t>(&opts.nthreads)
            ->default_value(opts.nthreads)
            ->implicit_value(opts.nthreads),
        "The number of worker threads. Half will request "
        "the Interest packets and half will process the Data packets");
    description.add_options()(
        "pipeline-type",
        po::value<string>(&pipelineType)->default_value(pipelineType),
        "The pipeline type. Available options: fixed, aimd");
    description.add_options()("pipeline-size",
                              po::value<uint16_t>(&opts.pipelineSize)
                                  ->default_value(opts.pipelineSize),
                              "The maximum pipeline size for `fixed` type or "
                              "the starting side for `aimd` type");
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
        cerr << "ERROR: no file path specified\n\n";
        usage(cerr, app, description);
        return 2;
    }

    if (opts.file.empty()) {
        cerr << "\nERROR: the file path argument cannot be an empty string\n";
        return 2;
    }

    if (al::to_lower_copy(pipelineType).compare("fixed") == 0) {
        opts.pipelineType = ndnc::PipelineType::fixed;
    } else if (al::to_lower_copy(pipelineType).compare("aimd") == 0) {
        opts.pipelineType = ndnc::PipelineType::aimd;
    } else {
        opts.pipelineType = ndnc::PipelineType::invalid;
    }

    if (opts.pipelineType == ndnc::PipelineType::invalid) {
        cerr << "ERROR: invalid pipeline type\n\n";
        usage(cout, app, description);
        return 2;
    }

    // if (!opts.influxdbaddr.empty() && !opts.influxdbname.empty()) {
    //     influxDBClient =
    //         new ndnc::InfluxDBClient(opts.influxdbaddr, opts.influxdbname);
    // }

    opts.nthreads = opts.nthreads % 2 == 1 ? opts.nthreads + 1 : opts.nthreads;

    face = new ndnc::Face();
    if (!face->openMemif(opts.mtu, opts.gqlserver, "ndncft-client"))
        return 2;

    if (!face->isValid()) {
        cerr << "ERROR: invalid face\n";
        return 2;
    }

    client = new ndnc::benchmark::ft::Runner(*face, opts);

    LOG_INFO("running...");

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

    for (auto wid = 0; wid < opts.nthreads / 2; ++wid) {
        workers.push_back(std::thread(
            &ndnc::benchmark::ft::Runner::requestFileContent, client, wid));
    }

    for (auto wid = 0; wid < opts.nthreads / 2; ++wid) {
        workers.push_back(std::thread(
            &ndnc::benchmark::ft::Runner::receiveFileContent, client,
            [&](uint64_t /*progress*/, uint64_t /*packets*/) {
                // TODO: submit data to influx db

                bar.set_progress(client->readCounters()->nByte);
                bar.set_option(option::PostfixText{
                    to_string(client->readCounters()->nByte) + "/" +
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
    double goodput = ((double)client->readCounters()->nByte * 8.0) /
                     (duration / std::chrono::nanoseconds(1)) * 1e9;

#ifndef NDEBUG
    double throughput = ((double)face->readCounters()->nRxBytes * 8.0) /
                        (duration / std::chrono::nanoseconds(1)) * 1e9;
#endif

    cout << "\n--- statistics --\n"
         << client->readCounters()->nInterest << " packets transmitted, "
         << client->readCounters()->nData << " packets received\n"
         << "average delay: " << client->readPipeCounters()->averageDelay()
         << "\n"
         << "goodput: " << humanReadableSize(goodput, 'b') << "/s"
#ifndef NDEBUG
         << ", throughput: " << humanReadableSize(throughput, 'b') << "/s\n";
#else
         << "\n";
#endif

#ifndef NDEBUG
    cout << "\n--- face statistics --\n"
         << face->readCounters()->nTxPackets << " packets transmitted, "
         << face->readCounters()->nRxPackets << " packets received\n"
         << face->readCounters()->nTxBytes << " bytes transmitted, "
         << face->readCounters()->nRxBytes << " bytes received "
         << "with " << face->readCounters()->nErrors << " errors\n";
#endif // NDEBUG
    cout << endl;

    cleanOnExit();
    return 0;
}
