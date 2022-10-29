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

#include "file.hpp"
#include "logger/logger.hpp"

namespace ndnc::posix {
File::File(std::shared_ptr<Consumer> consumer)
    : consumer_{consumer}, metadata_(nullptr), path_{} {
}

File::~File() {
    close();
}

int File::open(const char *path) {
    if (consumer_ == nullptr) {
        LOG_ERROR("null consumer object");
        return -1;
    }

    id_ = consumer_->registerConsumer();
    if (!getFileMetadata(path) || !isOpened()) {
        close();
        return -1;
    }

    if (!isOpened()) {
        return -1;
    }

    path_ = std::string(path);
    return 0;
}

bool File::isOpened() {
    return metadata_ != nullptr;
}

int File::close() {
    if (consumer_ == nullptr) {
        LOG_ERROR("null consumer object");
        return -1;
    }

    consumer_->unregisterConsumer(id_);
    metadata_ = nullptr;
    path_.clear();

    return 0;
}

int File::stat(const char *path, struct stat *buf) {
    if (isOpened()) {
        return fstat(buf);
    }

    auto res = open(path);
    if (res != 0) {
        return res;
    }

    metadata_->fstat(buf);
    return 0;
}

int File::fstat(struct stat *buf) {
    if (!isOpened()) {
        LOG_ERROR("file not opened");
        return -1;
    }

    metadata_->fstat(buf);
    return 0;
}

bool File::getFileMetadata(const char *path) {
    if (isOpened()) {
        LOG_DEBUG("file already opened");
        return false;
    }

    if (consumer_ == nullptr) {
        LOG_ERROR("null consumer object");
        return false;
    }

    auto interest =
        std::make_shared<ndn::Interest>(rdrDiscoveryNameFileRetrieval(
            std::string(path), consumer_->getNamePrefix()));
    interest->setCanBePrefix(true);
    interest->setMustBeFresh(true);

    auto data = consumer_->syncRequestDataFor(std::move(interest), id_);

    if (data == nullptr || !data->hasContent()) {
        LOG_ERROR("file metadata: invalid data");
        return false;
    }

    if (data->getContentType() == ndn::tlv::ContentType_Nack) {
        LOG_ERROR("unable to list file '%s'", path);
        return false;
    }

    metadata_ = std::make_shared<FileMetadata>(data->getContent());
    return true;
}
} // namespace ndnc::posix
