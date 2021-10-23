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

#ifndef NDNC_LOGGER_HPP
#define NDNC_LOGGER_HPP
#pragma once

#define ZF_LOG_VERSION_REQUIRED 4
#ifdef NDEBUG
#define ZF_LOG_SRCLOC ZF_LOG_SRCLOC_NONE
#endif

#include <algorithm>
#include <cctype>
#include <string>

#include "zf_log.h"

namespace ndnc {
#define LOG_LVL_DEBUG 2
#define LOG_LVL_INFO 3
#define LOG_LVL_WARN 4
#define LOG_LVL_ERROR 5
#define LOG_LVL_FATAL 6
#define LOG_LVL_NONE 0xFF

static inline void set_log_level(const std::string lvl) {
    std::string level = lvl;
    std::transform(level.begin(), level.end(), level.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (level.compare("debug")) {
        zf_log_set_output_level(LOG_LVL_DEBUG);
    } else if (level.compare("info")) {
        zf_log_set_output_level(LOG_LVL_INFO);
    } else if (level.compare("warn")) {
        zf_log_set_output_level(LOG_LVL_WARN);
    } else if (level.compare("error")) {
        zf_log_set_output_level(LOG_LVL_ERROR);
    } else if (level.compare("fatal")) {
        zf_log_set_output_level(LOG_LVL_FATAL);
    } else {
        zf_log_set_output_level(LOG_LVL_NONE);
    }
}

#define LOG_DEBUG ZF_LOGD
#define LOG_INFO ZF_LOGI
#define LOG_WARN ZF_LOGW
#define LOG_ERROR ZF_LOGE
#define LOG_FATAL ZF_LOGF
}; // namespace ndnc

#endif // NDNC_LOGGER_HPP
