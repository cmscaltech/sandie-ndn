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

#include "lib/posix/consumer.hpp"

#include "../common/ft-naming-scheme.hpp"
#include "lib/posix/dir.hpp"
#include "lib/posix/file-metadata.hpp"
#include "lib/posix/file-rdr.hpp"
#include "lib/posix/file-system-object.hpp"
#include "lib/posix/file.hpp"

namespace ndnc::app::filetransfer {
struct ClientOptions {
    struct ndnc::posix::ConsumerOptions consumer;

    std::vector<std::string> paths; // List of paths
    bool recursive;                 // Recursive list/copy paths
    size_t streams = 1;             // The number of streams
};
}; // namespace ndnc::app::filetransfer

namespace ndnc::app::filetransfer {
class Client : public std::enable_shared_from_this<Client> {
  public:
    using NotifyProgressStatus = std::function<void(uint64_t bytes)>;

  public:
    Client(std::shared_ptr<ndnc::posix::Consumer> consumer,
           ClientOptions options);
    ~Client();

    void stop();

    int openAllPaths();
    int closeAllPaths();
    std::vector<std::string> listAllFilePaths();
    uint64_t getByteCountOfAllOpenedFiles();

    // void
    // requestFileContent(std::shared_ptr<ndnc::posix::FileMetadata> metadata);
    // void
    // receiveFileContent(NotifyProgressStatus onProgress,
    //                    std::shared_ptr<ndnc::posix::FileMetadata> metadata);

  private:
    bool canContinue();

  private:
    std::mutex mutex_;
    std::atomic_bool isStopped_;
    std::atomic_bool error_;

    ClientOptions options_;
    std::shared_ptr<ndnc::posix::Consumer> consumer_;
    std::unordered_map<std::string, std::shared_ptr<ndnc::posix::File>> files_;
};
}; // namespace ndnc::app::filetransfer

#endif // NDNC_APP_FILE_TRANSFER_CLIENT_FT_CLIENT_HPP
