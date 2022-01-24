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
#include "pipeline/pipeline-interests-aimd.hpp"
#include "pipeline/pipeline-interests-fixed.hpp"

namespace ndnc {
namespace benchmark {
namespace ft {

struct ClientOptions {
    size_t mtu = 9000;                                // Dataroom size
    std::string gqlserver = "http://localhost:3030/"; // GraphQL server address
    std::string influxdbaddr = "";
    std::string influxdbname = "";

    // Interest lifetime
    ndn::time::milliseconds lifetime = ndn::time::seconds{2};

    std::string file;      // The file path
    uint16_t nthreads = 2; // The number of worker threads

    PipelineType pipelineType = PipelineType::fixed;
    uint16_t pipelineSize = 4096;
};

class Runner : public std::enable_shared_from_this<Runner> {
  public:
    using NotifyProgressStatus = std::function<void(uint64_t, uint64_t)>;

    struct Counters {
        std::atomic<uint64_t> nInterest = 0;
        std::atomic<uint64_t> nData = 0;
        std::atomic<uint64_t> nByte = 0;
    };

  public:
    explicit Runner(Face &face, ClientOptions options);
    ~Runner();

    void stop();
    void getFileInfo(uint64_t *size);

    std::shared_ptr<Counters> readCounters();

    void requestFileContent(int wid);
    void receiveFileContent(NotifyProgressStatus);

  private:
    bool canContinue();

    bool requestData(std::shared_ptr<ndn::Interest> &&);
    bool requestData(std::vector<std::shared_ptr<ndn::Interest>> &&, size_t);

  private:
    std::atomic_bool m_stop;
    std::atomic_bool m_error;
    std::atomic<uint64_t> m_nReceived;

    std::shared_ptr<ClientOptions> m_options;
    std::shared_ptr<Counters> m_counters;
    std::shared_ptr<PipelineInterests> m_pipeline;
    std::shared_ptr<FileMetadata> m_metadata;
};
}; // namespace ft
}; // namespace benchmark
}; // namespace ndnc

#endif // NDNC_APP_BENCHMARK_FT_CLIENT_HPP
