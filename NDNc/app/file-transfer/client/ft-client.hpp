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

#ifndef NDNC_APP_FILE_TRANSFER_CLIENT_FT_CLIENT_HPP
#define NDNC_APP_FILE_TRANSFER_CLIENT_FT_CLIENT_HPP

#include <atomic>
#include <vector>

#include "../common/file-metadata.hpp"
#include "../common/rdr-file.hpp"

#include "congestion-control/pipeline-interests-aimd.hpp"
#include "congestion-control/pipeline-interests-fixed.hpp"

namespace ndnc {
namespace ft {

struct ClientOptions {
    std::string namePrefix = NDNC_NAME_PREFIX_DEFAULT;

    size_t mtu = 9000;                                 // Dataroom size
    std::string gqlserver = "http://172.17.0.2:3030/"; // GraphQL server address
    // Interest lifetime
    ndn::time::milliseconds lifetime = ndn::time::seconds{2};

    std::vector<std::string> paths; // List of paths to be copied over NDN
    uint16_t nthreads = 2;          // The number of worker threads

    PipelineType pipelineType = PipelineType::aimd;
    size_t pipelineSize = 32768;
};

class Client : public std::enable_shared_from_this<Client> {
  public:
    using NotifyProgressStatus = std::function<void(uint64_t bytes)>;

  public:
    Client(ClientOptions options, std::shared_ptr<PipelineInterests> pipeline);
    ~Client();

    void stop();

    void openFile(std::shared_ptr<FileMetadata> metadata);
    void closeFile(std::shared_ptr<FileMetadata> metadata);
    void listFile(std::string path, std::shared_ptr<FileMetadata> &metadata);
    void listDir(std::string path,
                 std::vector<std::shared_ptr<FileMetadata>> &all);
    void listDirRecursive(std::string path,
                          std::vector<std::shared_ptr<FileMetadata>> &all);

    void requestFileContent(int wid, int wcount,
                            std::shared_ptr<FileMetadata> metadata);
    void receiveFileContent(NotifyProgressStatus onProgress,
                            std::atomic<uint64_t> &segmentsCount,
                            std::shared_ptr<FileMetadata> metadata);

  private:
    bool canContinue();

    std::shared_ptr<ndn::Data>
    syncRequestDataFor(std::shared_ptr<ndn::Interest> &&interest,
                       uint64_t id = 0);

    std::vector<std::shared_ptr<ndn::Data>>
    syncRequestDataFor(std::vector<std::shared_ptr<ndn::Interest>> &&interests,
                       uint64_t id = 0);

    bool asyncRequestDataFor(std::shared_ptr<ndn::Interest> &&interest,
                             uint64_t id = 0);

    bool
    asyncRequestDataFor(std::vector<std::shared_ptr<ndn::Interest>> &&interests,
                        uint64_t id = 0);

  private:
    std::atomic_bool m_stop;
    std::atomic_bool m_error;

    // TODO: This doesn't have to be a shared ptr
    std::shared_ptr<ClientOptions> m_options;

    std::shared_ptr<PipelineInterests> m_pipeline;

    // TODO: This doesn't have to be a shared ptr
    std::shared_ptr<std::unordered_map<std::string, uint64_t>> m_files;
};
}; // namespace ft
}; // namespace ndnc

#endif // NDNC_APP_FILE_TRANSFER_CLIENT_FT_CLIENT_HPP
