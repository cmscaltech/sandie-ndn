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
    : consumer_{consumer}, metadata_(nullptr), content_{}, contentIterator_{0} {
}

Dir::~Dir() {
    this->close();
}

int Dir::open(const char *path) {
    this->id_ = consumer_->registerConsumer();
    this->getDirMetadata(path);

    if (metadata_ == nullptr) {
        return -1;
    }

    if (!metadata_->isDir()) {
        return -ENOTDIR;
    }

    return 0;
}

int Dir::stat(struct stat *buf) {
    if (metadata_ == nullptr) {
        LOG_ERROR("dir not opened");
        return -1;
    }

    metadata_->fstat(buf);
    return 0;
}

int Dir::read(char *buf, int blen) {
    // Request the content of the directory
    if (content_.empty()) {
        auto contentBytes =
            (uint8_t *)malloc(8800 * (metadata_->getFinalBlockID() + 1));
        uint64_t offset = 0;

        if (contentBytes == nullptr) {
            LOG_FATAL("unable to allocate memory for reading dir content");
            return -1;
        }

        for (uint64_t i = 0, finalBlockId = 0; i <= finalBlockId; ++i) {
            auto interest = std::make_shared<ndn::Interest>(
                metadata_->getVersionedName().appendSegment(i));

            auto data = consumer_->syncRequestDataFor(std::move(interest), 0);

            if (data == nullptr || !data->hasContent()) {
                LOG_ERROR("invalid data");
                return -1;
            }

            if (data->getContentType() == ndn::tlv::ContentType_Nack) {
                LOG_FATAL("unable to get dir content");
                return -1;
            } else {
                if (!data->getFinalBlock()->isSegment()) {
                    LOG_ERROR(
                        "dir list data content FinalBlockId is not a segment");
                    return -1;
                } else {
                    finalBlockId = data->getFinalBlock()->toSegment();
                }
            }

            memcpy(contentBytes + offset, data->getContent().value(),
                   data->getContent().value_size());
            offset += data->getContent().value_size();
        }

        for (uint64_t i = 0, j = 0; i <= offset && j <= offset; ++j) {
            if (contentBytes[j] != '\0') {
                continue;
            }

            if (contentBytes[i] == '\0') {
                break;
            }

            content_.push_back(
                std::string((const char *)(contentBytes + i), j - i));

            i = j + 1;
        }

        free(contentBytes);
    }

    if (contentIterator_ < content_.size()) {
        memcpy(buf, content_[contentIterator_].c_str(), blen);
        ++contentIterator_;
    } else {
        buf[0] = '\0';
        content_.clear();
        contentIterator_ = 0;
    }

    return 0;
}

int Dir::close() {
    consumer_->unregisterConsumer(this->id_);

    if (metadata_ != nullptr) {
        metadata_ = nullptr;
    }

    content_.clear();

    return 0;
}

void Dir::getDirMetadata(const char *path) {
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
        LOG_ERROR("unable to list dir '%s'", path);
        return;
    }

    metadata_ = std::make_shared<FileMetadata>(data->getContent());
}
}; // namespace ndnc::posix
