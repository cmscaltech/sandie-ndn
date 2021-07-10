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

#include <vector>

#include "face/pipeline-interests.hpp"

namespace ndnc {
namespace benchmark {
namespace fileTransferClient {

struct Options {
    /**
     * @brief Name prefix
     *
     */
    std::string prefix;

    /**
     * @brief File name
     *
     */
    std::string filepath;

    /**
     * @brief File size in bytest
     *
     */

    uint64_t filesize = 0;

    /**
     * @brief Reading step. How much bytes to be read by a thread at once.
     *
     */
    uint64_t readChunk = 262144;

    /**
     * @brief Producer payload size
     *
     */
    size_t payloadSize = 1024;

    /**
     * @brief Number of threads to read the file
     *
     */

    uint16_t nthreads = 1;

    /**
     * @brief Interest lifetime
     *
     */
    ndn::time::milliseconds lifetime = ndn::time::seconds{1};
};

class Runner : public std::enable_shared_from_this<Runner> {
    using RxQueue = moodycamel::BlockingConcurrentQueue<ndn::Data>;

  public:
    Runner(Face &face, Options options);
    ~Runner();

    void run();
    void wait();
    void stop();

  private:
    void transfer(int index);
    int expressInterests(uint64_t offset, RxQueue *rxQueue);

  private:
    Options m_options;

    std::vector<std::thread> m_workers;
    std::atomic_bool m_stop;
    Pipeline *m_pipeline;
};
}; // namespace fileTransferClient
}; // namespace benchmark
}; // namespace ndnc

#endif // NDNC_APP_FILE_TRANSFER_BENCHMARK_CLIENT
