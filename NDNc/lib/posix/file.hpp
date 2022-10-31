/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2022 California Institute of Technology
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

#ifndef NDNC_LIB_POSIX_FILE_HPP
#define NDNC_LIB_POSIX_FILE_HPP

#include <mutex>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

#include "consumer.hpp"
#include "file-metadata.hpp"

namespace ndnc::posix {
class File {
  public:
    File(std::shared_ptr<Consumer> consumer);
    ~File();

    int open(const char *path);
    int close();
    int stat(const char *path, struct stat *buf);
    int fstat(struct stat *buf);
    ssize_t read(void *buf, off_t offset, size_t blen);

  private:
    bool isOpened();
    bool getFileMetadata(const char *path);
    uint64_t getConsumerId();

  private:
    std::shared_ptr<Consumer> consumer_;
    std::shared_ptr<FileMetadata> metadata_;
    std::string path_;

    std::unordered_map<std::thread::id, uint64_t> consumer_ids_;
    std::mutex mutex_;
};
}; // namespace ndnc::posix

#endif // NDNC_LIB_POSIX_FILE_HPP
