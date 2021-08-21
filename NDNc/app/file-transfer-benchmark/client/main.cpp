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

#include <nlohmann/json.hpp>

#include "file-transfer-client.hpp"

using namespace std;
namespace po = boost::program_options;

static ndnc::benchmark::fileTransferClient::Runner *client;

void handler(sig_atomic_t signal) {
    if (NULL != client) {
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
       << " [options]\nNote: This application needs --prefix, --file-path and "
          "--file-size arguments to be specified\n\n"
       << desc;
}

int main(int argc, char *argv[]) {
    ndnc::benchmark::fileTransferClient::Options opts;

    po::options_description description("Options", 120);
    description.add_options()(
        "interest-lifetime,l",
        po::value<ndn::time::milliseconds::rep>()
            ->default_value(1000)
            ->implicit_value(1000),
        "Interest lifetime in milliseconds. Specify a non-negative value");
    description.add_options()("prefix,p", po::value<string>(&opts.prefix),
                              "The NDN Name prefix of all Interests expressed "
                              "by this consumer. Specify a non-empty string");
    description.add_options()("file-path,f", po::value<string>(&opts.filePath),
                              "The path to the file to be copied over NDN. "
                              "Specify a non-empty string");
    description.add_options()("file-size,s",
                              po::value<uint64_t>(&opts.fileSize),
                              "The file size in bytes to be copied over NDN. "
                              "Specify a non-negative value");
    description.add_options()("payload-size,l",
                              po::value<size_t>(&opts.dataPayloadSize)
                                  ->default_value(opts.dataPayloadSize)
                                  ->implicit_value(opts.dataPayloadSize),
                              "The producer's payload size. Used by this "
                              "consumer to compute Interests");
    description.add_options()(
        "read-chunk,c",
        po::value<uint64_t>(&opts.fileReadChunk)
            ->default_value(opts.fileReadChunk)
            ->implicit_value(opts.fileReadChunk),
        "The number of bytes to be read in one request. Specify "
        "a non-negative integer higher than 0");
    description.add_options()(
        "nthreads,t",
        po::value<uint16_t>(&opts.nThreads)
            ->default_value(opts.nThreads)
            ->implicit_value(opts.nThreads),
        "The number of threads to concurrently read the file. Specify a "
        "non-negative value higher than 0");
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

    string app = argv[0];

    if (vm.count("help") > 0) {
        usage(cout, app, description);
        return 0;
    }

    if (vm.count("prefix") == 0) {
        usage(cerr, app, description);
        cerr << "\nERROR: the option '--prefix' is required but missing\n";
        return 2;
    }

    if (opts.prefix.empty()) {
        cerr << "\nERROR: invalid value for option '--prefix'\n";
        return 2;
    }

    if (vm.count("file-path") == 0) {
        usage(cerr, app, description);
        cerr << "\nERROR: the option '--file-path' is required but missing\n";
        return 2;
    }

    if (opts.filePath.empty()) {
        cerr << "\nERROR: invalid value for option '--file-path'\n";
        return 2;
    }

    if (vm.count("file-size") == 0) {
        usage(cerr, app, description);
        cerr << "\nERROR: the option '--file-size' is required but missing\n";
        return 2;
    }

    if (opts.fileSize == 0) {
        cerr << "\nERROR: invalid value for option '--file-size'\n";
        return 2;
    }

    if (vm.count("interest-lifetime") > 0) {
        opts.interestLifetime = ndn::time::milliseconds(
            vm["interest-lifetime"].as<ndn::time::milliseconds::rep>());
    }

    if (opts.interestLifetime < ndn::time::milliseconds{0}) {
        cerr << "\nERROR: invalid value for option '--interest-lifetime'\n";
        return 2;
    }

    if (opts.fileReadChunk <= 0) {
        cerr << "\nERROR: invalid value for option '--read-chunk'\n";
    }

    signal(SIGINT, handler);
    signal(SIGABRT, handler);

    std::cout
        << "TRACE: Starting NDNc File Transfer Client benchmarking tool...\n";

    ndnc::Face *face = new ndnc::Face();
#ifndef __APPLE__
    face->openMemif(9000, "ndnc-benchmark-test", "http://172.17.0.2:3030/");
#endif

    if (!face->isValid()) {
        cerr << "ERROR: Could not create face\n";
        return -1;
    }

    client = new ndnc::benchmark::fileTransferClient::Runner(*face, opts);

    auto start_time = std::chrono::high_resolution_clock::now();
    std::atomic<uint64_t> totalProgress = 0;

    client->run([&](uint64_t progress) {
        totalProgress.fetch_add(progress, std::memory_order_release);
    });

    client->wait();

    auto duration = std::chrono::high_resolution_clock::now() - start_time;
    double goodput = ((double)totalProgress * 8.0) /
                     (duration / std::chrono::nanoseconds(1)) * 1000000000;

#ifdef DEBUG
    double throughput = ((double)face->readCounters().nRxBytes * 8.0) /
                        (duration / std::chrono::nanoseconds(1)) * 1000000000;
#endif

    std::stringstream ss;
    ss << "{\n";

    if (NULL != client) {
        ss << "\"client counters\": { ";
        ss << "\"nInterest\": " << client->readCounters().nInterest << ", ";
        ss << "\"nData\": " << client->readCounters().nData << ", ";
        ss << "\"Goodput\": \"" << humanReadableSize(goodput, 'b');
#ifdef DEBUG
        ss << "/s\", \"Throughput\": \"" << humanReadableSize(throughput, 'b');
#endif
        ss << "/s\" },\n";
    }
#ifdef DEBUG
    if (NULL != face) {
        ss << "\"face counters\": { ";
        ss << "\"nTxPackets\": " << face->readCounters().nTxPackets << ", ";
        ss << "\"nTxBytes\": " << face->readCounters().nTxBytes << ", ";
        ss << "\"nRxPackets\": " << face->readCounters().nRxPackets << ", ";
        ss << "\"nRxBytes\": " << face->readCounters().nRxBytes << ", ";
        ss << "\"nErrors\": " << face->readCounters().nErrors << " },\n";
    }
#endif // DEBUG

    ss << "\"client options\": { ";
    ss << "\"nThreads\": " << opts.nThreads << ", ";
    ss << "\"prefix\": \"" << opts.prefix << "\", ";
    ss << "\"interest lifetime\": " << opts.interestLifetime.count() << ", ";
    ss << "\"payload size\": \"" << humanReadableSize(opts.dataPayloadSize)
       << "\", ";
    ss << "\"file path\": \"" << opts.filePath << "\", ";
    ss << "\"file size\": \"" << humanReadableSize(opts.fileSize) << "\", ";
    ss << "\"file read chunk\": \"" << humanReadableSize(opts.fileReadChunk)
       << "\" }\n";
    ss << "}\n";

    cout << std::setfill('*') << std::setw(80) << "\n";
    cout << nlohmann::json::parse(ss.str()).dump(4) << "\n";
    cout << std::setfill('*') << std::setw(80) << "\n";

    if (NULL != client) {
        delete client;
    }

    if (NULL != face) {
        delete face;
    }

    return 0;
}
