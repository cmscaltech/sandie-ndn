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

#ifndef NDNC_APP_BENCHMARK_FT_COMMON_NAMING_SCHEME_HPP
#define NDNC_APP_BENCHMARK_FT_COMMON_NAMING_SCHEME_HPP

#include <string>

#include <ndn-cxx/name.hpp>

namespace ndnc {
namespace benchmark {
static const std::string namePrefixUri = std::string("/ndnc/ft");
static const ndn::Name namePrefix = ndn::Name(namePrefixUri);
static const uint8_t namePrefixNoComponents = 2;

static const ndn::Name::Component metadataNameSuffixComponent =
    ndn::Name::Component::fromEscapedString("32=metadata");

inline static const ndn::Name getNameForMetadata(std::string filePath) {
    return ndn::Name(namePrefixUri + filePath)
        .append(metadataNameSuffixComponent);
}

inline static const ndn::Name getNameWithSegment(std::string filePath,
                                                 uint64_t segmentNo,
                                                 uint64_t fileVersion) {
    return ndn::Name(namePrefixUri + filePath)
        .appendVersion(fileVersion)
        .appendSegment(segmentNo);
}

inline static const std::string
getFilePathFromMetadataName(const ndn::Name name) {
    return name.getPrefix(-1).getSubName(namePrefixNoComponents).toUri();
}

inline static const std::string
getFilePathFromNameWithSegment(const ndn::Name name) {
    return name.getPrefix(-2).getSubName(namePrefixNoComponents).toUri();
}

inline static bool isMetadataName(const ndn::Name name) {
    return !name.at(-1).isSegment() &&
           name.at(-1) == metadataNameSuffixComponent;
}
}; // namespace benchmark
}; // namespace ndnc

#endif // NDNC_APP_BENCHMARK_FT_COMMON_NAMING_SCHEME_HPP
