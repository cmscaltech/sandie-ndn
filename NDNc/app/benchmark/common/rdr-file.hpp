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

#ifndef NDNC_APP_COMMON_RDR_FILE_HPP
#define NDNC_APP_COMMON_RDR_FILE_HPP

#include "ft-naming-scheme.hpp"

namespace ndnc {
/**
 * @brief Get RDR discovery Interest Name for FILE RETRIEVAL
 * https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR
 * https://github.com/yoursunny/ndn6-tools/blob/main/file-server.md#protocol-details
 *
 * @param path The file path
 * @return const ndn::Name The NDN Name
 */
inline static const ndn::Name
rdrDiscoveryInterestNameFromFilePath(std::string path) {
    return ndn::Name(NDNC_NAME_PREFIX)
        .append(path)
        .append(ndn::Name::Component::fromEscapedString("32=metadata"));
}

/**
 * @brief Get the file path from a FILE RETRIEVAL RDR discovery packet Name
 * https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR
 * https://github.com/yoursunny/ndn6-tools/blob/main/file-server.md#protocol-details
 *
 * @param name
 * @return const std::string
 */
inline static const std::string
rdrFilePathFromDiscoveryInterestName(const ndn::Name name) {
    return name.getPrefix(-1)
        .getSubName(NDNC_NAME_PREFIX_NO_COMPONENTS)
        .toUri();
}

/**
 * @brief Check if the NDN Name corresponds to a RDR discovery packet Name
 * https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR
 * https://github.com/yoursunny/ndn6-tools/blob/main/file-server.md#protocol-details
 *
 * @param name The NDN Name
 * @return true
 * @return false
 */
inline static bool isRDRDiscovery(const ndn::Name name) {
    return !name.at(-1).isSegment() &&
           name.at(-1) ==
               ndn::Name::Component::fromEscapedString("32=metadata");
}
}; // namespace ndnc

#endif // NDNC_APP_COMMON_RDR_FILE_HPP
