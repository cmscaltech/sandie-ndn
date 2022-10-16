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

#include "consumer.hpp"
#include "logger/logger.hpp"

namespace ndnc::posix {
Consumer::Consumer(ConsumerOptions options)
    : options_{options}, face_{nullptr}, pipeline_{nullptr}, is_valid_{false},
      error_{false} {
    this->openFace();
    this->openPipeline();
}

Consumer::~Consumer() {
    if (pipeline_ != nullptr && !pipeline_->isClosed()) {
        pipeline_->close();
    }
}

bool Consumer::isValid() {
    return this->is_valid_ && !this->error_ && !pipeline_->isClosed();
}

void Consumer::openFace() {
    this->face_ = std::make_unique<ndnc::face::Face>();
    this->is_valid_ =
        face_->connect(options_.mtu, options_.gqlserver, options_.name);
}

void Consumer::openPipeline() {
    if (!this->is_valid_) {
        return;
    }

    switch (options_.pipelineType) {
    case ndnc::PipelineType::aimd:
        this->pipeline_ = std::make_shared<ndnc::PipelineInterestsAimd>(
            *face_, options_.pipelineSize);
        break;
    case ndnc::PipelineType::fixed:
    default:
        this->pipeline_ = std::make_shared<ndnc::PipelineInterestsFixed>(
            *face_, options_.pipelineSize);
    }
}

uint64_t Consumer::registerConsumer() {
    return pipeline_->registerConsumer();
}

void Consumer::unregisterConsumer(const uint64_t id) {
    pipeline_->unregisterConsumer(id);
}

std::shared_ptr<ndn::Data>
Consumer::syncRequestDataFor(std::shared_ptr<ndn::Interest> &&interest,
                             uint64_t id) {
    interest->setInterestLifetime(options_.interestLifetime);

    // Insert the Interest packet in the pipeline
    if (!pipeline_->pushInterest(id, std::move(interest))) {
        LOG_FATAL("unable to push Interest packet to pipeline");
        error_ = true;
        return nullptr;
    }

    std::shared_ptr<ndn::Data> pkt(nullptr);
    // Wait for the response from the pipeline
    while (this->isValid(), !pipeline_->popData(id, pkt)) {}

    return pkt;
}

std::vector<std::shared_ptr<ndn::Data>> Consumer::syncRequestDataFor(
    std::vector<std::shared_ptr<ndn::Interest>> &&interests, uint64_t id) {
    auto npkts = interests.size();

    for (auto interest : interests) {
        interest->setInterestLifetime(options_.interestLifetime);
    }

    if (!pipeline_->pushInterestBulk(id, std::move(interests))) {
        LOG_FATAL("unable to push Interest packets to pipeline");
        error_ = true;
        return {};
    }

    std::vector<std::shared_ptr<ndn::Data>> pkts(npkts);

    for (; npkts > 0; --npkts) {
        std::shared_ptr<ndn::Data> pkt(nullptr);

        while (this->isValid() && !pipeline_->popData(id, pkt)) {}

        if (pkt == nullptr) {
            return {};
        }

        if (!pkt->getName().at(-1).isSegment()) {
            LOG_FATAL("last name component of Data packet is not a segment");
            error_ = true;
            return {};
        }

        try {
            pkts[pkt->getName().at(-1).toSegment()] = pkt;
        } catch (ndn::Name::Error &error) {
            LOG_FATAL("unable to get the segment from the Data Name");
            error_ = true;
            return {};
        }
    }

    return pkts;
}

bool Consumer::asyncRequestDataFor(std::shared_ptr<ndn::Interest> &&interest,
                                   uint64_t id) {
    interest->setInterestLifetime(options_.interestLifetime);

    if (!pipeline_->pushInterest(id, std::move(interest))) {
        LOG_FATAL("unable to push Interest packet to pipeline");
        error_ = true;
        return false;
    }

    return true;
}

bool Consumer::asyncRequestDataFor(
    std::vector<std::shared_ptr<ndn::Interest>> &&interests, uint64_t id) {

    for (auto interest : interests) {
        interest->setInterestLifetime(options_.interestLifetime);
    }

    if (!pipeline_->pushInterestBulk(id, std::move(interests))) {
        LOG_FATAL("unable to push Interest packets to pipeline");
        error_ = true;
        return false;
    }

    return true;
}

size_t Consumer::getData(std::vector<std::shared_ptr<ndn::Data>> &pkts,
                         uint64_t id) {
    return pipeline_->popDataBulk(id, pkts);
}

ndn::Name Consumer::getNamePrefix() {
    return options_.prefix;
}

ndnc::PipelineCounters Consumer::getCounters() {
    return pipeline_->getCounters();
}
} // namespace ndnc::posix
