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

#ifndef NDNC_APP_FILE_TRANSFER_BENCHMARK_CLIENT
#define NDNC_APP_FILE_TRANSFER_BENCHMARK_CLIENT

#include <atomic>
#include <vector>

#include "face/pipeline-interests.hpp"

namespace ndnc {
namespace benchmark {
namespace fileTransferClient {

struct Options {
    /**
     * @brief Number of threads to read the file
     *
     */

    uint16_t nThreads = 1;

    /**
     * @brief NDN Name prefix
     *
     */
    std::string prefix;

    /**
     * @brief Interest lifetime
     *
     */
    ndn::time::milliseconds interestLifetime = ndn::time::seconds{1};

    /**
     * @brief Producer payload size
     *
     */
    size_t dataPayloadSize = 1024;

    /**
     * @brief File name
     *
     */
    std::string filePath;

    /**
     * @brief File size in bytes
     *
     */

    uint64_t fileSize = 0;

    /**
     * @brief Reading step. How much bytes to be read by a thread at once.
     *
     */
    uint64_t fileReadChunk = 262144;
};

class Runner : public std::enable_shared_from_this<Runner> {
    using RxQueue = moodycamel::BlockingConcurrentQueue<ndn::Data>;
    using NotifyProgressStatus = std::function<void(uint64_t)>;

  public:
    /**
     * @brief Consumer counters
     *
     */
    struct Counters {
        std::atomic<uint64_t> nInterest = 0;
        std::atomic<uint64_t> nData = 0;
    };

  public:
    explicit Runner(Face &face, Options options);
    ~Runner();

    void run(NotifyProgressStatus onProgress);
    void wait();
    void stop();

    Counters &readCounters();

  private:
    void transfer(int index, NotifyProgressStatus onProgress);
    int expressInterests(uint64_t offset, RxQueue *rxQueue);

  private:
    Options m_options;
    Counters m_counters;

    std::vector<std::thread> m_workers;
    std::atomic_bool m_stop;
    Pipeline *m_pipeline;
};
}; // namespace fileTransferClient
}; // namespace benchmark
}; // namespace ndnc

#endif // NDNC_APP_FILE_TRANSFER_BENCHMARK_CLIENT
