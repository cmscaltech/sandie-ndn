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

#ifndef NDNC_APP_COMMON_FILE_TRANSFER_NAMING_SCHEME_HPP
#define NDNC_APP_COMMON_FILE_TRANSFER_NAMING_SCHEME_HPP

#include <ndn-cxx/name.hpp>
#include <string>

/**
 * @brief NDN Name related constants in NDNc file transfer (ft) applications
 *
 */
namespace ndnc {
// NDNc Name prefix
static const ndn::Name NDNC_NAME_PREFIX = ndn::Name("/ndnc/ft");
// NDNc Name prefix components count
static const uint8_t NDNC_NAME_PREFIX_NO_COMPONENTS = 2;
}; // namespace ndnc

#endif // NDNC_APP_COMMON_FILE_TRANSFER_NAMING_SCHEME_HPP
