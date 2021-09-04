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

#ifndef NDNC_APP_BENCHMARK_FT_CLIENT_HPP
#define NDNC_APP_BENCHMARK_FT_CLIENT_HPP

#include <atomic>
#include <vector>

#include "../common/file-metadata.hpp"
#include "../common/naming-scheme.hpp"
#include "face/pipeline-interests.hpp"

namespace ndnc {
namespace benchmark {
namespace ft {

struct ClientOptions {
    size_t mtu = 9000;                                // Dataroom size
    std::string gqlserver = "http://localhost:3030/"; // GraphQL server address
    ndn::time::milliseconds lifetime =
        ndn::time::seconds{2}; // Interest lifetime

    std::string file;      // File path
    uint16_t nthreads = 1; // Number of worker threads to request packets
};

class Runner : public std::enable_shared_from_this<Runner> {
  public:
    using RxQueue = moodycamel::BlockingConcurrentQueue<ndn::Data>;
    using NotifyProgressStatus = std::function<void(uint64_t)>;

    struct Counters {
        std::atomic<uint64_t> nInterest = 0;
        std::atomic<uint64_t> nData = 0;
    };

  public:
    explicit Runner(Face &face, ClientOptions options);
    ~Runner();

    void run(NotifyProgressStatus onProgress);
    void wait();
    void stop();

    Counters &readCounters();

  private:
    bool getFileMetadata();
    void getFileContent(int tid, NotifyProgressStatus onProgress);

    int expressInterests(std::shared_ptr<ndn::Interest> interest,
                         RxQueue *rxQueue);

  private:
    ClientOptions m_options;
    Counters m_counters;

    const struct FileMetadata *m_fileMetadata;
    uint64_t m_finalBlockId;
    uint8_t m_chunk;

    std::vector<std::thread> m_workers;
    std::atomic_bool m_stop;
    Pipeline *m_pipeline;
};
}; // namespace ft
}; // namespace benchmark
}; // namespace ndnc

#endif // NDNC_APP_BENCHMARK_FT_CLIENT_HPP
