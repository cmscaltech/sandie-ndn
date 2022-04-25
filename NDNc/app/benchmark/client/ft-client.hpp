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
#include "../common/rdr-file.hpp"

#include "pipeline/pipeline-interests-aimd.hpp"
#include "pipeline/pipeline-interests-fixed.hpp"

namespace ndnc {
namespace benchmark {
namespace ft {

struct ClientOptions {
    size_t mtu = 9000;                                 // Dataroom size
    std::string gqlserver = "http://172.17.0.2:3030/"; // GraphQL server address
    std::string influxdbaddr = "";
    std::string influxdbname = "";

    // Interest lifetime
    ndn::time::milliseconds lifetime = ndn::time::seconds{2};

    std::string file;      // The file path
    uint16_t nthreads = 2; // The number of worker threads

    PipelineType pipelineType = PipelineType::aimd;
    uint16_t pipelineSize = 32768;
};

class Runner : public std::enable_shared_from_this<Runner> {
  public:
    using NotifyProgressStatus = std::function<void()>;

  public:
    explicit Runner(face::Face &face, ClientOptions options);
    ~Runner();

    void stop();

    bool getFileMetadata(FileMetadata &metadata);
    void requestFileContent(int wid, FileMetadata metadata);
    void receiveFileContent(NotifyProgressStatus onProgress,
                            std::atomic<uint64_t> &bytesCount,
                            std::atomic<uint64_t> &segmentsCount,
                            uint64_t finalBlockId);

    std::shared_ptr<PipelineInterests::Counters> readCounters();

  private:
    bool canContinue();

    bool requestData(std::shared_ptr<ndn::Interest> &&);
    bool requestData(std::vector<std::shared_ptr<ndn::Interest>> &&);

  private:
    std::atomic_bool m_stop;
    std::atomic_bool m_error;

    std::shared_ptr<ClientOptions> m_options;
    std::shared_ptr<PipelineInterests> m_pipeline;
};
}; // namespace ft
}; // namespace benchmark
}; // namespace ndnc

#endif // NDNC_APP_BENCHMARK_FT_CLIENT_HPP
