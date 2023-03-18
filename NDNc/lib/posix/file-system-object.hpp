/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2023 California Institute of Technology
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

#ifndef NDNC_LIB_POSIX_FSO_HPP
#define NDNC_LIB_POSIX_FSO_HPP

#include <stdexcept>

#include "consumer.hpp"
#include "file-metadata.hpp"

namespace ndnc::posix {

inline static std::shared_ptr<FileMetadata>
getMetadata(const char *path, std::shared_ptr<Consumer> consumer) {
    if (consumer == nullptr) {
        throw std::exception_ptr();
    }

    auto interest =
        std::make_shared<ndn::Interest>(rdrDiscoveryNameFileRetrieval(
            std::string(path), consumer->getNamePrefix()));
    interest->setCanBePrefix(true);
    interest->setMustBeFresh(true);

    auto id = consumer->registerConsumer();
    auto data = consumer->syncRequestDataFor(std::move(interest), id);
    consumer->unregisterConsumer(id);

    if (data == nullptr || !data->hasContent()) {
        LOG_ERROR("invalid data");
        return nullptr;
    }

    if (data->getContentType() == ndn::tlv::ContentType_Nack) {
        LOG_ERROR("unable to list path '%s'", path);
        return nullptr;
    }

    return std::make_shared<FileMetadata>(data->getContent());
}
}; // namespace ndnc::posix

#endif // NDNC_LIB_POSIX_FSO_HPP
