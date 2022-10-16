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
    : consumer_{consumer}, metadata_(nullptr) {
}

File::~File() {
    this->close();
}

int File::fstat(struct stat *buf) {
    if (metadata_ != nullptr) {
        LOG_ERROR("file not opened");
        return -1;
    }

    metadata_->stat(buf);

    return 0;
}

int File::open(const char *path) {
    this->id_ = consumer_->registerConsumer();
    this->getFileMetadata(path);

    if (metadata_ == nullptr) {
        return -1;
    }

    return 0;
}

int File::close() {
    consumer_->unregisterConsumer(this->id_);

    if (metadata_ != nullptr) {
        metadata_ = nullptr;
    } else {
        LOG_WARN("try to close unopened file");
    }

    return 0;
}

void File::getFileMetadata(const char *path) {
    if (metadata_ != nullptr) {
        LOG_WARN("file already opened");
        return;
    }

    auto interest =
        std::make_shared<ndn::Interest>(rdrDiscoveryNameFileRetrieval(
            std::string(path), consumer_->getNamePrefix()));
    interest->setCanBePrefix(true);
    interest->setMustBeFresh(true);

    auto data = consumer_->syncRequestDataFor(std::move(interest), id_);

    if (data == nullptr || !data->hasContent()) {
        LOG_ERROR("invalid data");
        return;
    }

    if (data->getContentType() == ndn::tlv::ContentType_Nack) {
        LOG_ERROR("unable to list file '%s'", path);
        return;
    }

    metadata_ = std::make_shared<FileMetadata>(data->getContent());
}
} // namespace ndnc::posix
