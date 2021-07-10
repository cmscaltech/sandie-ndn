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

#include "file-transfer-client.hpp"

using namespace std;
namespace po = boost::program_options;

static ndnc::benchmark::fileTransferClient::Runner *client;

void handler(sig_atomic_t signal) {
    if (NULL != client) {
        client->stop();
    }
}

static void usage(ostream &os, const string &app,
                  const po::options_description &desc) {
    os << "Usage: " << app
       << " [options]\nNote: This application needs --prefix, --filepath and "
          "--filesize arguments to be specified\n\n"
       << desc;
}

int main(int argc, char *argv[]) {
    ndnc::benchmark::fileTransferClient::Options opts;

    po::options_description description("Options", 120);
    description.add_options()(
        "lifetime,l",
        po::value<ndn::time::milliseconds::rep>()
            ->default_value(1000)
            ->implicit_value(1000),
        "Interests lifetime in milliseconds. Specify a non-negative value");
    description.add_options()("prefix,p", po::value<string>(&opts.prefix),
                              "The NDN Name prefix of all Interests expressed "
                              "by this consumer. Specify a non-empty string");
    description.add_options()("filepath,f", po::value<string>(&opts.filepath),
                              "The path to the file to be copied over NDN. "
                              "Specify a non-empty string");
    description.add_options()("filesize,s", po::value<uint64_t>(&opts.filesize),
                              "The file size in bytes to be copied over NDN. "
                              "Specify a non-negative value");
    description.add_options()("payload-size,l",
                              po::value<size_t>(&opts.payloadSize)
                                  ->default_value(opts.payloadSize)
                                  ->implicit_value(opts.payloadSize),
                              "The producer's payload size. Used by this "
                              "consumer to compute Interests");
    description.add_options()(
        "chunk,c",
        po::value<uint64_t>(&opts.readChunk)
            ->default_value(opts.readChunk)
            ->implicit_value(opts.readChunk),
        "The number of bytes to be read in one request. Specify "
        "a non-negative integer");
    description.add_options()(
        "nthreads,t",
        po::value<uint16_t>(&opts.nthreads)
            ->default_value(opts.nthreads)
            ->implicit_value(opts.nthreads),
        "The number of threads to concurrently read the file. Specify a "
        "non-negative value higher than 1");
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

    if (vm.count("filepath") == 0) {
        usage(cerr, app, description);
        cerr << "\nERROR: the option '--filepath' is required but missing\n";
        return 2;
    }

    if (opts.filepath.empty()) {
        cerr << "\nERROR: invalid value for option '--filepath'\n";
        return 2;
    }

    if (vm.count("filesize") == 0) {
        usage(cerr, app, description);
        cerr << "\nERROR: the option '--filesize' is required but missing\n";
        return 2;
    }

    if (opts.filesize == 0) {
        cerr << "\nERROR: invalid value for option '--filesize'\n";
        return 2;
    }

    if (vm.count("lifetime") > 0) {
        opts.lifetime = ndn::time::milliseconds(
            vm["lifetime"].as<ndn::time::milliseconds::rep>());
    }

    if (opts.lifetime < ndn::time::milliseconds{0}) {
        cerr << "\nERROR: onvalid value for option '--lifetime'\n";
        return 2;
    }

    signal(SIGINT, handler);
    signal(SIGABRT, handler);

    std::cout
        << "TRACE: Starting NDNc File Transfer Client benchmarking tool...\n";

    ndnc::Face *face = new ndnc::Face();
    if (!face->isValid()) {
        cerr << "ERROR: Could not create face\n";
        return -1;
    }

    client = new ndnc::benchmark::fileTransferClient::Runner(*face, opts);

    client->run();
    client->wait();

    if (NULL != client) {
        delete client;
    }

    if (NULL != face) {
        delete face;
    }

    return 0;
}
