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
#include "lib/posix/consumer.hpp"
#include "logger/logger.hpp"

namespace po = boost::program_options;
namespace al = boost::algorithm;

static std::shared_ptr<ndnc::posix::Consumer> consumer;
static std::unique_ptr<ndnc::app::filetransfer::Client> client;
static std::vector<std::thread> workers;

static void programUsage(std::ostream &os, const std::string &app,
                         const po::options_description &desc) {
    os << "Usage: " << app
       << " [options]\nNote: This application requires only one of the "
          "arguments: --list, --copy, to be specified\n\n"
       << desc;
}

static void programOptions(int argc, char *argv[],
                           ndnc::app::filetransfer::ClientOptions &opts,
                           bool &copy, bool &list, bool &recursive) {
    po::options_description description("Options", 120);

    std::string pipelineType = "aimd";
    std::string prefix = ndnc::app::filetransfer::NDNC_NAME_PREFIX_DEFAULT;

    description.add_options()(
        "copy,c",
        po::value<std::vector<std::string>>(&opts.paths)->multitoken(),
        "Copy a list files or directories over NDN");
    description.add_options()("gqlserver",
                              po::value<std::string>(&opts.consumer.gqlserver)
                                  ->default_value(opts.consumer.gqlserver),
                              "The GraphQL server address");
    description.add_options()(
        "lifetime",
        po::value<ndn::time::milliseconds::rep>()->default_value(
            opts.consumer.interestLifetime.count()),
        "The Interest lifetime in milliseconds. Specify a positive integer");
    description.add_options()(
        "list,l",
        po::value<std::vector<std::string>>(&opts.paths)->multitoken(),
        "List one or more files or directories");
    description.add_options()(
        "mtu",
        po::value<size_t>(&opts.consumer.mtu)->default_value(opts.consumer.mtu),
        "Dataroom size. Specify a positive integer between 64 and 9000");
    description.add_options()(
        "name-prefix", po::value<std::string>(&prefix)->default_value(prefix),
        "The NDN Name prefix this consumer application publishes its "
        "Interest packets. Specify a non-empty string");
    description.add_options()(
        "pipeline-type",
        po::value<std::string>(&pipelineType)->default_value(pipelineType),
        "The pipeline type. Available options: fixed, aimd");
    description.add_options()("pipeline-size",
                              po::value<size_t>(&opts.consumer.pipelineSize)
                                  ->default_value(opts.consumer.pipelineSize),
                              "The maximum pipeline size for `fixed` type or "
                              "the initial ssthresh for `aimd` type");
    description.add_options()("recursive,r", po::bool_switch(&recursive),
                              "Set recursive copy or list of directories");
    description.add_options()(
        "streams,s",
        po::value<size_t>(&opts.streams)->default_value(opts.streams),
        "The number of streams. Specify a positive integer between 1 and 16");

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
        if (opts.consumer.mtu < 64 || opts.consumer.mtu > 9000) {
            std::cerr << "ERROR: invalid MTU size\n\n";
            programUsage(std::cout, app, description);
            exit(2);
        }
    }

    if (vm.count("gqlserver") > 0) {
        if (opts.consumer.gqlserver.empty()) {
            std::cerr << "ERROR: empty gqlserver argument value\n\n";
            programUsage(std::cout, app, description);
            exit(2);
        }
    }

    if (vm.count("lifetime") > 0) {
        opts.consumer.interestLifetime = ndn::time::milliseconds(
            vm["lifetime"].as<ndn::time::milliseconds::rep>());
    }

    if (opts.consumer.interestLifetime < ndn::time::milliseconds{0}) {
        std::cerr << "ERROR: negative lifetime argument value\n\n";
        programUsage(std::cout, app, description);
        exit(2);
    }

    if ((vm.count("copy") == 0 && vm.count("list") == 0) ||
        (vm.count("copy") != 0 && vm.count("list") != 0)) {
        programUsage(std::cerr, app, description);
        exit(2);
    } else {
        copy = vm.count("copy") != 0;
        list = vm.count("list") != 0;
    }

    if (opts.paths.empty()) {
        std::cerr
            << "\nERROR: the paths argument cannot be an empty string\n\n";
        exit(2);
    }

    if (al::to_lower_copy(pipelineType).compare("fixed") == 0) {
        opts.consumer.pipelineType = ndnc::PipelineType::fixed;
    } else if (al::to_lower_copy(pipelineType).compare("aimd") == 0) {
        opts.consumer.pipelineType = ndnc::PipelineType::aimd;
    } else {
        opts.consumer.pipelineType = ndnc::PipelineType::invalid;
    }

    if (opts.consumer.pipelineType == ndnc::PipelineType::invalid) {
        std::cerr << "ERROR: invalid pipeline type\n\n";
        programUsage(std::cout, app, description);
        exit(2);
    }

    if (vm.count("name-prefix") > 0) {
        if (opts.consumer.prefix.empty()) {
            std::cerr << "ERROR: empty name prefix value\n\n";
            programUsage(std::cout, app, description);
            exit(2);
        }
    }

    opts.consumer.prefix = ndn::Name(prefix);

    if (vm.count("streams") > 0) {
        if (opts.streams < 1 || opts.streams > 16) {
            std::cerr << "ERROR: invalid streams value\n\n";
            programUsage(std::cout, app, description);
            exit(2);
        }
    }
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
    ndnc::app::filetransfer::ClientOptions opts;
    opts.consumer.name = "ndncft-client";
    bool copy = false, list = false, recursive = false;

    programOptions(argc, argv, opts, copy, list, recursive);

    // Init consumer
    consumer = std::make_shared<ndnc::posix::Consumer>(opts.consumer);
    if (!consumer->isValid()) {
        LOG_FATAL("invalid consumer");
        return -2;
    }

    // Init client
    client = std::make_unique<ndnc::app::filetransfer::Client>(consumer, opts);

    // Get all file information
    std::vector<std::shared_ptr<ndnc::posix::FileMetadata>> metadata{};
    for (auto path : opts.paths) {
        std::shared_ptr<ndnc::posix::FileMetadata> md;
        client->listFile(path, md);

        if (md == nullptr) {
            continue;
        }

        if (md->isFile()) {
            metadata.push_back(md);
        } else {
            std::vector<std::shared_ptr<ndnc::posix::FileMetadata>> partial{};

            if (recursive) {
                client->listDirRecursive(path, partial);
            } else {
                client->listDir(path, partial);
            }

            metadata.insert(std::end(metadata), std::begin(partial),
                            std::end(partial));
        }
    }

    uint64_t totalByteCount = 0;
    uint64_t totalFileCount = 0;

    std::cout << "\n";
    for (auto md : metadata) {
        if (md->isFile()) {
            std::cout << ndnc::posix::rdrFileUri(md->getVersionedName(),
                                                 opts.consumer.prefix)
                      << "\n";
            totalByteCount += md->getFileSize();
            totalFileCount += 1;
        } else if (list) {
            std::cout << ndnc::posix::rdrDirUri(md->getVersionedName(),
                                                opts.consumer.prefix)
                      << "\n";
            totalFileCount += 1;
        }
    }

    std::cout << "\ntotal " << totalFileCount << "\n";
    std::cout << "total size " << totalByteCount << " bytes\n\n";

    if (list || totalByteCount == 0) {
        programTerminate();
        return 0;
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

    auto receiveWorker = [&currentByteCount, &totalByteCount, &bar, &opts,
                          &metadata](size_t wid) {
        for (size_t i = wid; i < metadata.size(); i += opts.streams) {
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
                metadata[i]);
        }
    };

    auto requestWorker = [&opts, &metadata](size_t wid) {
        for (size_t i = wid; i < metadata.size(); i += opts.streams) {
            client->requestFileContent(metadata[i]);
        }
    };

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < metadata.size(); ++i) {
        client->openFile(metadata[i]);
    }

    for (size_t i = 0; i < opts.streams; ++i) {
        workers.push_back(std::thread(receiveWorker, i));
        workers.push_back(std::thread(requestWorker, i));
    }

    for (auto it = workers.begin(); it != workers.end(); ++it) {
        if (it->joinable()) {
            it->join();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    bar.set_option(indicators::option::PrefixText{"Transfer completed "});
    bar.mark_as_completed();

    for (size_t i = 0; i < metadata.size(); ++i) {
        if (metadata[i]->isDir()) {
            continue;
        }

        std::cout << termcolor::bold << termcolor::green << "✔ Downloaded file "
                  << ndnc::posix::rdrFileUri(metadata[i]->getVersionedName(),
                                             opts.consumer.prefix)
                  << std::endl;
        std::cout << termcolor::reset;

        client->closeFile(metadata[i]);
    }

    auto duration = end - start;
    double goodput = ((double)totalByteCount * 8.0) /
                     (duration / std::chrono::nanoseconds(1)) * 1e9;

    auto statistics = consumer->getCounters();

    std::cout << termcolor::bold << "\n--- statistics ---\n"
              << statistics.tx << " interest packets transmitted, "
              << statistics.rx << " data packets received, "
              << statistics.timeout << " timeout retries\n"
              << "average delay: " << statistics.getAverageDelay() << "\n"
              << "goodput: " << ndnc::app::filetransfer::binaryPrefix(goodput)
              << "bit/s"
              << "\n\n";
    std::cout << termcolor::reset;

    programTerminate();
    return 0;
}
