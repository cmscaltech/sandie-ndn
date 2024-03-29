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

#ifndef NDNC_LIB_POSIX_FILE_RDR_HPP
#define NDNC_LIB_POSIX_FILE_RDR_HPP

#include <ndn-cxx/name.hpp>
#include <string>

namespace ndnc::posix {
static const ndn::Name::Component metadataComponent =
    ndn::Name::Component::fromEscapedString("32=metadata");
static const ndn::Name::Component lsComponent =
    ndn::Name::Component::fromEscapedString("32=ls");
}; // namespace ndnc::posix

namespace ndnc::posix {
/**
 * @brief Get RDR discovery packet Name for FILE RETRIEVAL
 * https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR
 * https://github.com/yoursunny/ndn6-tools/blob/main/file-server.md#protocol-details
 *
 * @param path The file path
 * @param prefix The Name prefix
 * @return const ndn::Name The NDN packet Name
 */
inline static const ndn::Name
rdrDiscoveryNameFileRetrieval(const std::string path, const ndn::Name prefix) {
    return prefix.deepCopy().append(path).append(metadataComponent);
}

/**
 * @brief Get RDR discovery packet Name for DIRECTORY LISTING
 * https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR
 * https://github.com/yoursunny/ndn6-tools/blob/main/file-server.md#protocol-details
 *
 * @param path The dir path
 * @param prefix The Name prefix
 * @return const ndn::Name The NDN packet Name
 */
inline static const ndn::Name
rdrDiscoveryNameDirListing(const std::string path, const ndn::Name prefix) {
    return prefix.deepCopy()
        .append(path)
        .append(lsComponent)
        .append(metadataComponent);
}

/**
 * @brief Check if the NDN Name corresponds to a RDR discovery packet Name
 * https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR
 * https://github.com/yoursunny/ndn6-tools/blob/main/file-server.md#protocol-details
 *
 * @param name The NDN packet Name
 * @return true
 * @return false
 */
inline static bool isRDRDiscoveryName(const ndn::Name name) {
    return !name.at(-1).isSegment() && name.at(-1) == metadataComponent;
}

/**
 * @brief Get the file path from a FILE RETRIEVAL RDR discovery packet Name
 * https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR
 * https://github.com/yoursunny/ndn6-tools/blob/main/file-server.md#protocol-details
 *
 * @param name The NDN packet Name
 * @param prefix The Name prefix
 * @return const std::string The file path
 */
inline static const std::string rdrFileUri(const ndn::Name name,
                                           const ndn::Name prefix) {
    return name.getPrefix(-1).getSubName(prefix.size()).toUri();
}

/**
 * @brief Get the directory path from a DIRECTORY LISTING RDR discovery packet
 * Name https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR
 * https://github.com/yoursunny/ndn6-tools/blob/main/file-server.md#protocol-details
 *
 * @param name The NDN packet Name
 * @param prefix The Name prefix
 * @return const std::string The directory path
 */
inline static const std::string rdrDirUri(const ndn::Name name,
                                          const ndn::Name prefix) {
    return name.getPrefix(-2).getSubName(prefix.size()).toUri();
}
}; // namespace ndnc::posix

#endif // NDNC_LIB_POSIX_FILE_RDR_HPP
