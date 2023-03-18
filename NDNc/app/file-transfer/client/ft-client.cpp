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
#include <queue>

#include "ft-client.hpp"
#include "logger/logger.hpp"

namespace ndnc::app::filetransfer {
Client::Client(std::shared_ptr<ndnc::posix::Consumer> consumer,
               ClientOptions options)
    : isStopped_{false}, error_{false}, options_{options}, consumer_{consumer},
      files_{} {
}

Client::~Client() {
    this->stop();
}

bool Client::canContinue() {
    return !isStopped_ && !error_ && consumer_->isValid();
}

void Client::stop() {
    isStopped_ = true;

    if (consumer_ != nullptr) {
        consumer_->stop();
    }

    this->closeAllPaths();
}

int Client::openAllPaths() {
    if (consumer_ == nullptr) {
        LOG_FATAL("null consumer object");

        error_ = true;
        return -1;
    }

    std::queue<std::string> paths;
    for (auto path : options_.paths) {
        paths.push(path);
    }

    auto addPath = [](std::string path, bool recursive) {
        if (recursive) {
            return true;
        } else {
            return path.back() != '/';
        }
    };

    while (!paths.empty()) {
        auto root = paths.front();
        paths.pop();

        auto metadata = ndnc::posix::getMetadata(root.c_str(), consumer_);

        if (metadata == nullptr) {
            continue;
        }

        if (metadata->isFile()) {
            auto file =
                std::make_shared<ndnc::posix::File>(consumer_, metadata);

            mutex_.lock();
            files_[root] = file;
            mutex_.unlock();
        } else {
            auto dir = ndnc::posix::Dir(consumer_, metadata);

            // Read the content of the current directory
            std::vector<std::string> dirPaths{};
            int len = 4096;
            auto buf = (char *)malloc(len * sizeof(char));

            while (0 == dir.read(buf, len) && buf[0] != '\0') {
                dirPaths.push_back(std::string(buf));
                memset(buf, 0, len);
            }

            dir.close();

            for (auto path : dirPaths) {
                if (addPath(path, options_.recursive)) {
                    paths.push(root + path);
                }
            }
        }
    }

    std::lock_guard<std::mutex> guard(mutex_);
    return files_.size();
}

int Client::closeAllPaths() {
    std::lock_guard<std::mutex> guard(mutex_);

    for (auto &it : files_) {
        it.second->close();
    }

    files_.clear();
    return 0;
}

std::vector<std::string> Client::listAllFilePaths() {
    std::vector<std::string> paths{};

    mutex_.lock();
    for (auto it : files_) {
        paths.push_back(it.first);
    }
    mutex_.unlock();

    std::sort(paths.begin(), paths.end());
    return paths;
}

uint64_t Client::getByteCountOfAllOpenedFiles() {
    uint64_t byteCount = 0;

    mutex_.lock();
    for (auto it : files_) {
        struct stat info;

        if (it.second->fstat(&info) != 0) {
            LOG_FATAL("unable to fstat");

            error_ = true;
            return 0;
        }

        byteCount += info.st_size;
    }
    mutex_.unlock();

    return byteCount;
}

// void Client::requestFileContent(
//     std::shared_ptr<ndnc::posix::FileMetadata> metadata) {
//     uint64_t npkts = 64;
//     uint64_t id = files_->at(metadata->getVersionedName().toUri());

//     for (uint64_t segmentNo = 0;
//          segmentNo <= metadata->getFinalBlockID() && this->canContinue();) {

//         // if (m_pipeline->getQueuedInterestsCount() > 65536) {
//         //     // backoff; plenty of work to be done by the pipeline
//         //     continue;
//         // }

//         std::vector<std::shared_ptr<ndn::Interest>> pkts;
//         pkts.reserve(npkts);

//         for (uint64_t nextSegment = segmentNo, n = 0;
//              nextSegment <= metadata->getFinalBlockID() && n < npkts;
//              ++nextSegment, ++n) {

//             auto interest = std::make_shared<ndn::Interest>(
//                 metadata->getVersionedName().deepCopy().appendSegment(
//                     nextSegment));

//             pkts.emplace_back(std::move(interest));
//         }

//         if (!consumer_->asyncRequestDataFor(std::move(pkts), id)) {
//             error_ = true;
//             return;
//         }

//         segmentNo += npkts;
//     }
// }

// void Client::receiveFileContent(
//     NotifyProgressStatus onProgress,
//     std::shared_ptr<ndnc::posix::FileMetadata> metadata) {
//     uint64_t bytesCount = 0;
//     uint64_t segmentsCount = 0;
//     uint64_t id = files_->at(metadata->getVersionedName().toUri());

//     while (this->canContinue() &&
//            segmentsCount <= metadata->getFinalBlockID()) {

//         size_t npkts = 16;
//         std::vector<std::shared_ptr<ndn::Data>> pkts(npkts);
//         npkts = consumer_->getData(pkts, id);

//         if (npkts == 0) {
//             continue;
//         }

//         for (size_t i = 0; i < npkts; ++i) {
//             if (pkts[i] == nullptr) {
//                 LOG_FATAL("pipeline error on receive file content");
//                 error_ = true;
//                 return;
//             }

//             bytesCount += pkts[i]->getContent().value_size();
//         }

//         segmentsCount += npkts;

//         if (bytesCount > 2097152) {
//             onProgress(bytesCount);
//             bytesCount = 0;
//         }
//     }

//     onProgress(bytesCount);
// }
}; // namespace ndnc::app::filetransfer
