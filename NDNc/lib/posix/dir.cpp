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

#include "dir.hpp"

namespace ndnc::posix {
Dir::Dir(std::shared_ptr<Consumer> consumer)
    : consumer_{consumer},
      metadata_(nullptr), path_{}, content_{}, onReadNextEntryPos_{0} {
}

Dir::~Dir() {
    close();
}

int Dir::open(const char *path) {
    if (consumer_ == nullptr) {
        return -1;
    }

    if (!getDirMetadata(path) || !isOpened()) {
        LOG_ERROR("null consumer object");
        return -1;
    }

    if (!metadata_->isDir()) {
        close();
        return -ENOTDIR;
    }

    path_ = std::string(path);
    return 0;
}

bool Dir::isOpened() {
    return metadata_ != nullptr;
}

int Dir::stat(struct stat *buf) {
    if (!isOpened()) {
        LOG_ERROR("dir not opened");
        return -1;
    }

    metadata_->fstat(buf);
    return 0;
}

int Dir::read(char *buf, int blen) {
    if (content_.empty() && !getDirContent()) {
        return -1;
    }

    if (onReadNextEntryPos_ < content_.size()) {
        memcpy(buf, content_[onReadNextEntryPos_].c_str(), blen);
        ++onReadNextEntryPos_;
    } else {
        buf[0] = '\0';
        content_.clear();
        onReadNextEntryPos_ = 0;
    }

    return 0;
}

int Dir::close() {
    if (consumer_ == nullptr) {
        LOG_ERROR("null consumer object");
        return -1;
    }

    metadata_ = nullptr;

    content_.clear();
    onReadNextEntryPos_ = 0;
    path_.clear();

    return 0;
}

bool Dir::getDirMetadata(const char *path) {
    if (isOpened()) {
        LOG_DEBUG("dir already opened");
        return false;
    }

    if (consumer_ == nullptr) {
        LOG_ERROR("null consumer object");
        return -1;
    }

    auto interest =
        std::make_shared<ndn::Interest>(rdrDiscoveryNameFileRetrieval(
            std::string(path), consumer_->getNamePrefix()));
    interest->setCanBePrefix(true);
    interest->setMustBeFresh(true);

    auto id = consumer_->registerConsumer();
    auto data = consumer_->syncRequestDataFor(std::move(interest), id);
    consumer_->unregisterConsumer(id);

    if (data == nullptr || !data->hasContent()) {
        LOG_ERROR("dir metadata: invalid data");
        return false;
    }

    if (data->getContentType() == ndn::tlv::ContentType_Nack) {
        LOG_ERROR("unable to list dir '%s'", path);
        return false;
    }

    metadata_ = std::make_shared<FileMetadata>(data->getContent());
    return true;
}

bool Dir::getDirContent() {
    if (!isOpened()) {
        LOG_ERROR("trying to read the contents of an unopened dir");
        return false;
    }

    if (consumer_ == nullptr) {
        LOG_ERROR("null consumer object");
        return -1;
    }

    uint8_t *bytes = nullptr;
    uint64_t offset = 0;

    auto id = consumer_->registerConsumer();

    for (uint64_t i = 0, finalBlockId = 0; i <= finalBlockId; ++i) {
        auto interest = std::make_shared<ndn::Interest>(
            metadata_->getVersionedName().appendSegment(i));

        auto data = consumer_->syncRequestDataFor(std::move(interest), id);

        if (data == nullptr || !data->hasContent()) {
            LOG_ERROR("read dir contents: invalid data");
            consumer_->unregisterConsumer(id);
            return false;
        }

        if (data->getContentType() == ndn::tlv::ContentType_Nack) {
            LOG_FATAL("unable to get the contents of dir '%s'", path_.c_str());
            consumer_->unregisterConsumer(id);
            return false;
        } else {
            if (!data->getFinalBlock()->isSegment()) {
                LOG_ERROR(
                    "dir list data content FinalBlockId is not a segment");
                consumer_->unregisterConsumer(id);
                return false;
            } else {
                finalBlockId = data->getFinalBlock()->toSegment();
            }
        }

        if (bytes == nullptr) {
            bytes = (uint8_t *)malloc(8800 * finalBlockId);

            if (bytes == nullptr) {
                LOG_FATAL("unable to allocate memory for reading dir contents");
                return false;
            }
        }

        memcpy(bytes + offset, data->getContent().value(),
               data->getContent().value_size());
        offset += data->getContent().value_size();
    }

    consumer_->unregisterConsumer(id);

    for (uint64_t i = 0, j = 0; i <= offset && j <= offset; ++j) {
        if (bytes[j] != '\0') {
            continue;
        }

        if (bytes[i] == '\0') {
            break;
        }

        content_.push_back(std::string((const char *)(bytes + i), j - i));

        i = j + 1;
    }

    free(bytes);
    return true;
}
}; // namespace ndnc::posix
