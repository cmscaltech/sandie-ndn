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
#include "indicators/indicators.hpp"
#include "logger/logger.hpp"

namespace po = boost::program_options;
namespace al = boost::algorithm;

static std::unique_ptr<ndnc::face::Face> face;
static std::unique_ptr<ndnc::benchmark::ft::Runner> client;
static std::vector<std::thread> workers;

static void programUsage(std::ostream &os, const std::string &app,
                         const po::options_description &desc) {
    os << "Usage: " << app
       << " [options]\nNote: This application needs --paths argument "
          "specified\n\n"
       << desc;
}

static void programOptions(int argc, char *argv[],
                           ndnc::benchmark::ft::ClientOptions &opts) {
    po::options_description description("Options", 120);
    std::string pipelineType = "fixed";

    description.add_options()(
        "gqlserver",
        po::value<std::string>(&opts.gqlserver)->default_value(opts.gqlserver),
        "The GraphQL server address");
    description.add_options()(
        "lifetime",
        po::value<ndn::time::milliseconds::rep>()->default_value(
            opts.lifetime.count()),
        "The Interest lifetime in milliseconds. Specify a positive integer");
    description.add_options()(
        "mtu", po::value<size_t>(&opts.mtu)->default_value(opts.mtu),
        "Dataroom size. Specify a positive integer between 64 and 9000");
    description.add_options()(
        "name-prefix",
        po::value<std::string>(&opts.namePrefix)
            ->default_value(opts.namePrefix),
        "The NDN Name prefix this consumer application publishes its "
        "Interest packets. Specify a non-empty string");
    description.add_options()(
        "nthreads",
        po::value<uint16_t>(&opts.nthreads)
            ->default_value(opts.nthreads)
            ->implicit_value(opts.nthreads),
        "The number of worker threads. Half will request the Interest packets "
        "and half will process the Data packets");
    description.add_options()(
        "paths", po::value<std::vector<std::string>>(&opts.paths)->multitoken(),
        "The list of paths to be copied over NDN");
    description.add_options()(
        "pipeline-type",
        po::value<std::string>(&pipelineType)->default_value(pipelineType),
        "The pipeline type. Available options: fixed, aimd");
    description.add_options()("pipeline-size",
                              po::value<uint16_t>(&opts.pipelineSize)
                                  ->default_value(opts.pipelineSize),
                              "The maximum pipeline size for `fixed` type or "
                              "the initial ssthresh for `aimd` type");

    description.add_options()("help,h", "Print this help message and exit");

    po::variables_map vm;
    try {
        po::store(
            po::command_line_parser(argc, argv).options(description).run(), vm);
        po::notify(vm);
    } catch (const po::error &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        exit(2);
    } catch (const boost::bad_any_cast &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        exit(2);
    }

    const std::string app = argv[0];

    if (vm.count("help") > 0) {
        programUsage(std::cout, app, description);
        exit(0);
    }

    if (vm.count("mtu") > 0) {
        if (opts.mtu < 64 || opts.mtu > 9000) {
            std::cerr << "ERROR: invalid MTU size\n\n";
            programUsage(std::cout, app, description);
            exit(2);
        }
    }

    if (vm.count("gqlserver") > 0) {
        if (opts.gqlserver.empty()) {
            std::cerr << "ERROR: empty gqlserver argument value\n\n";
            programUsage(std::cout, app, description);
            exit(2);
        }
    }

    if (vm.count("lifetime") > 0) {
        opts.lifetime = ndn::time::milliseconds(
            vm["lifetime"].as<ndn::time::milliseconds::rep>());
    }

    if (opts.lifetime < ndn::time::milliseconds{0}) {
        std::cerr << "ERROR: negative lifetime argument value\n\n";
        programUsage(std::cout, app, description);
        exit(2);
    }

    if (vm.count("paths") == 0) {
        std::cerr << "ERROR: no paths specified\n\n";
        programUsage(std::cerr, app, description);
        exit(2);
    }

    if (opts.paths.empty()) {
        std::cerr
            << "\nERROR: the paths argument cannot be an empty string\n\n";
        exit(2);
    }

    if (al::to_lower_copy(pipelineType).compare("fixed") == 0) {
        opts.pipelineType = ndnc::PipelineType::fixed;
    } else if (al::to_lower_copy(pipelineType).compare("aimd") == 0) {
        opts.pipelineType = ndnc::PipelineType::aimd;
    } else {
        opts.pipelineType = ndnc::PipelineType::invalid;
    }

    if (opts.pipelineType == ndnc::PipelineType::invalid) {
        std::cerr << "ERROR: invalid pipeline type\n\n";
        programUsage(std::cout, app, description);
        exit(2);
    }

    if (vm.count("name-prefix") > 0) {
        if (opts.namePrefix.empty()) {
            std::cerr << "ERROR: empty name prefix value\n\n";
            programUsage(std::cout, app, description);
            exit(2);
        }
    }

    opts.nthreads = opts.nthreads % 2 == 1 ? opts.nthreads + 1 : opts.nthreads;
}

static void programTerminate() {
    if (client != nullptr) {
        client->stop();

        for (auto it = workers.begin(); it != workers.end(); ++it) {
            if (it->joinable()) {
                it->join();
            }
        }
    }
}

static void signalHandler(sig_atomic_t signum) {
    programTerminate();
    exit(signum);
}

int main(int argc, char *argv[]) {
    // Register signal handler for program clean exit
    signal(SIGINT, signalHandler);

    // Parse command line arguments
    ndnc::benchmark::ft::ClientOptions opts;
    programOptions(argc, argv, opts);

    // Open memif face
    face = std::make_unique<ndnc::face::Face>();
    if (!face->connect(opts.mtu, opts.gqlserver, "ndncft-client")) {
        return 2;
    }

    // Init client
    client = std::make_unique<ndnc::benchmark::ft::Runner>(*face, opts);

    std::vector<ndnc::FileMetadata> metadata{};
    uint64_t totalByteCount = 0;

    // Get Metadata for all paths
    for (auto path : opts.paths) {
        ndnc::FileMetadata md{};

        if (!client->getFileMetadata(path, md)) {
            LOG_WARN("unable to get File Metadata for: '%s. will skip...",
                     path.c_str());
            continue;
        }

        if (!md.isFile()) {
            LOG_WARN("'%s' is a directory. will skip...", path.c_str());
            continue;
        }

        if (md.getFileSize() == 0) {
            LOG_WARN("'%s' file is empty. will skip...", path.c_str());
            continue;
        }

        totalByteCount += md.getFileSize();
        metadata.push_back(md);
    }

    if (totalByteCount == 0) {
        LOG_WARN("no bytes to be transfered");
        programTerminate();
        return -2;
    }

    // Prepare progress bar indicator for terminal output
    indicators::BlockProgressBar bar{
        indicators::option::BarWidth{80},
        indicators::option::PrefixText{"Transferring "},
        indicators::option::ShowPercentage{true},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true},
        indicators::option::MaxProgress{totalByteCount}};
    indicators::show_console_cursor(true);

    std::atomic<uint64_t> currentByteCount = 0;

    auto receiveWorker = [&currentByteCount, &totalByteCount,
                          &bar](ndnc::FileMetadata metadata,
                                std::atomic<uint64_t> &segmentCount) {
        client->receiveFileContent(
            [&](uint64_t bytes) {
                if (bar.is_completed()) {
                    return;
                }

                currentByteCount += bytes;

                bar.set_option(indicators::option::PostfixText{
                    "[" + std::to_string(currentByteCount) + "/" +
                    std::to_string(totalByteCount) + "]"});
                bar.set_progress(currentByteCount);
                bar.tick();
            },
            std::ref(segmentCount), metadata.getFinalBlockID());
    };

    auto requestWorker = [&opts](ndnc::FileMetadata metadata, int wid) {
        client->requestFileContent(wid, opts.nthreads / 2,
                                   metadata.getFinalBlockID(),
                                   metadata.getVersionedName());
    };

    // Copy all paths one at a time
    auto start = std::chrono::high_resolution_clock::now();

    for (auto i = 0; i < static_cast<int>(metadata.size()); ++i) {
        std::atomic<uint64_t> segmentCount = 0;

        for (int wid = 0; wid < opts.nthreads / 2; ++wid) {
            workers.push_back(std::thread(receiveWorker, metadata[i],
                                          std::ref(segmentCount)));
            workers.push_back(std::thread(requestWorker, metadata[i], wid));
        }

        for (auto it = workers.begin(); it != workers.end(); ++it) {
            if (it->joinable()) {
                it->join();
            }
        }

        // Store information
    }

    auto end = std::chrono::high_resolution_clock::now();

    bar.set_option(indicators::option::PrefixText{"Transfer completed "});
    bar.mark_as_completed();

    auto duration = end - start;
    double goodput = ((double)totalByteCount * 8.0) /
                     (duration / std::chrono::nanoseconds(1)) * 1e9;

    std::cout << "\n--- statistics --\n"
              << client->readCounters()->nTxPackets
              << " interest packets transmitted, "
              << client->readCounters()->nRxPackets
              << " data packets received, " << client->readCounters()->nTimeouts
              << " packets retransmitted on timeout\n"
              << "average delay: " << client->readCounters()->averageDelay()
              << "\n"
              << "goodput: " << binaryPrefix(goodput) << "bit/s"
              << "\n\n";

    programTerminate();
    return 0;
}
