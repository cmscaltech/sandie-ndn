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

#include <algorithm>
#include <cmath>

#include "file.hpp"
#include "logger/logger.hpp"

namespace ndnc::posix {
File::File(std::shared_ptr<Consumer> consumer,
           std::shared_ptr<FileMetadata> metadata = nullptr)
    : consumer_{consumer}, metadata_{metadata}, reporter_{nullptr}, path_{},
      consumer_ids_{} {

    if (!consumer_->getOptions().influxdb.empty()) {
        this->reporter_ = std::make_unique<ndnc::MeasurementsReporter>(
            256, consumer->getOptions().to_string());
        this->reporter_->init("xrd", consumer->getOptions().influxdb);
    }
}

File::~File() {
    close();
}

int File::open(const char *path) {
    if (consumer_ == nullptr) {
        LOG_ERROR("null consumer object");
        return -1;
    }

    if (isOpened()) {
        LOG_DEBUG("file already opened");
        return 0;
    }

    try {
        this->metadata_ = getMetadata(path, this->consumer_);
    } catch (...) {
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
    if (reporter_ != nullptr) {
        reporter_.reset();
    }

    if (consumer_ == nullptr) {
        LOG_ERROR("null consumer object");
        return -1;
    }

    metadata_ = nullptr;
    path_.clear();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = consumer_ids_.begin(); it != consumer_ids_.end(); ++it) {
            consumer_->unregisterConsumer(it->second);
        }
        consumer_ids_.clear();
    }

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

ssize_t File::read(void *buf, off_t offset, size_t blen) {
    if (!isOpened()) {
        return -1;
    }
    auto indexFirstSegment = offset / metadata_->getSegmentSize();
    auto indexLastSegment = ceil(
        (offset + blen) / static_cast<double>(metadata_->getSegmentSize()));

    std::vector<std::shared_ptr<ndn::Interest>> pkts;
    for (auto segment = indexFirstSegment; segment < indexLastSegment;
         ++segment) {
        auto interest = std::make_shared<ndn::Interest>(
            metadata_->getVersionedName().deepCopy().appendSegment(segment));
        pkts.emplace_back(std::move(interest));
    }

    auto response =
        consumer_->syncRequestDataFor(std::move(pkts), getConsumerId());
    if (response.empty()) {
        return -1;
    }

    std::sort(response.begin(), response.end(),
              [](const std::shared_ptr<ndn::Data> data1,
                 const std::shared_ptr<ndn::Data> data2) {
                  return data1->getName().at(-1).toSegment() <
                         data2->getName().at(-1).toSegment();
              });

    ssize_t n = 0;
    auto blen_copy = blen;

    auto copyDataContent = [&](const ndn::Block &block, size_t off) {
        auto len = std::min(block.value_size() - off, blen);
        memcpy((uint8_t *)buf + n, block.value() + off, len);
        n += len;
        blen -= len;
    };

    auto it = response.begin();
    copyDataContent(it->get()->getContent(),
                    offset % metadata_->getSegmentSize());
    ++it;

    for (; it != response.end(); ++it) {
        copyDataContent(it->get()->getContent(), 0);
    }

    if (reporter_ != nullptr) {
        auto counters = consumer_->getCounters();
        reporter_->write(counters.tx, counters.rx, blen_copy,
                         counters.getAverageDelay().count());
    }

    return n;
}

uint64_t File::getConsumerId() {
    auto tid = std::this_thread::get_id();

    std::lock_guard<std::mutex> lock(mutex_);

    if (consumer_ids_.find(tid) == consumer_ids_.end()) {
        consumer_ids_[tid] = consumer_->registerConsumer();
    }

    return consumer_ids_[tid];
}
} // namespace ndnc::posix
