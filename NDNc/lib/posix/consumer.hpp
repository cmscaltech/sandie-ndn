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

#ifndef NDNC_LIB_POSIX_CONSUMER_HPP
#define NDNC_LIB_POSIX_CONSUMER_HPP

#include <vector>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/util/time.hpp>

#include "congestion-control/pipeline-interests-aimd.hpp"
#include "congestion-control/pipeline-interests-fixed.hpp"

namespace ndnc::posix {
struct ConsumerOptions {
    // App name
    std::string name = "ndncposixconsumer";
    // GraphQL server address
    std::string gqlserver = "http://172.17.0.2:3030/";
    // Dataroom size
    size_t mtu = 9000;

    // Name prefix
    ndn::Name prefix = ndn::Name("/ndnc/xrootd");
    // Interest lifetime
    ndn::time::milliseconds interestLifetime{2000};

    // Pipeline type
    PipelineType pipelineType = PipelineType::aimd;
    // Pipeline size
    size_t pipelineSize = 32768;
};
}; // namespace ndnc::posix

namespace ndnc::posix {
class Consumer : public std::enable_shared_from_this<Consumer> {
  public:
    Consumer(ConsumerOptions options);
    ~Consumer();

    bool isValid();

    uint64_t registerConsumer();

    void unregisterConsumer(const uint64_t id);

    std::shared_ptr<ndn::Data>
    syncRequestDataFor(std::shared_ptr<ndn::Interest> &&interest, uint64_t id);

    std::vector<std::shared_ptr<ndn::Data>>
    syncRequestDataFor(std::vector<std::shared_ptr<ndn::Interest>> &&interests,
                       uint64_t id);

    bool asyncRequestDataFor(std::shared_ptr<ndn::Interest> &&interest,
                             uint64_t id);

    bool
    asyncRequestDataFor(std::vector<std::shared_ptr<ndn::Interest>> &&interests,
                        uint64_t id);

    size_t getData(std::vector<std::shared_ptr<ndn::Data>> &pkts, uint64_t id);

  public:
    ndn::Name getNamePrefix();
    ndnc::PipelineCounters getCounters();

  private:
    void openFace();
    void openPipeline();

  private:
    ConsumerOptions options_;
    std::unique_ptr<ndnc::face::Face> face_;
    std::shared_ptr<ndnc::PipelineInterests> pipeline_;

    std::atomic_bool is_valid_;
    std::atomic_bool error_;
};
}; // namespace ndnc::posix

#endif // NDNC_LIB_POSIX_CONSUMER_HPP
