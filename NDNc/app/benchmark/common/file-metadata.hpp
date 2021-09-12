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

#ifndef NDNC_APP_BENCHMARK_FT_COMMON_FILE_METADATA_HPP
#define NDNC_APP_BENCHMARK_FT_COMMON_FILE_METADATA_HPP

#include <cstdint>
#include <math.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include "naming-scheme.hpp"

namespace ndnc {
namespace benchmark {
static const uint32_t STATX_REQUIRED =
    STATX_TYPE | STATX_MODE | STATX_MTIME | STATX_SIZE;
static const uint32_t STATX_OPTIONAL = STATX_ATIME | STATX_CTIME | STATX_BTIME;

class FileMetadata {
    enum {
        TtSegmentSize = 0xF500,
        TtSize = 0xF502,
        TtMode = 0xF504,
        TtAtime = 0xF506,
        TtBtime = 0xF508,
        TtCtime = 0xF50A,
        TtMtime = 0xF50C,
    };

  public:
    FileMetadata() {}
    FileMetadata(uint64_t segmentSize) { this->m_segmentSize = segmentSize; }
    FileMetadata(const ndn::Block &content) : m_st{} { decode(content); }
    ~FileMetadata() {}

    bool prepare(const std::string path) {
        int res = syscall(__NR_statx, -1, path.c_str(), 0,
                          STATX_REQUIRED | STATX_OPTIONAL, &this->m_st);

        if (res != 0 || !has(STATX_REQUIRED)) {
            return false;
        }

        this->m_versionedName = getNameForMetadata(path).getPrefix(-1);
        this->m_versionedName.appendVersion(timestampToNano(m_st.stx_mtime));

        this->m_lastSegment =
            (uint64_t)(ceil((double)m_st.stx_size / m_segmentSize));

        return true;
    }

    ndn::Block encode() {
        ndn::Block content(ndn::tlv::Content);

        content.push_back(this->m_versionedName.wireEncode());

        ndn::Block finalBlockId{ndn::tlv::FinalBlockId};
        finalBlockId.push_back(
            ndn::name::Component::fromSegment(this->m_lastSegment)
                .wireEncode());
        finalBlockId.encode();
        content.push_back(finalBlockId);

        content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
            TtSegmentSize, this->m_segmentSize));
        content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
            TtSize, this->m_st.stx_size));

        content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
            TtMode, this->m_st.stx_mode));

        if (has(STATX_ATIME)) {
            content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
                TtAtime, timestampToNano(this->m_st.stx_atime)));
        }
        if (has(STATX_BTIME)) {
            content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
                TtBtime, timestampToNano(this->m_st.stx_btime)));
        }
        if (has(STATX_CTIME)) {
            content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
                TtCtime, timestampToNano(this->m_st.stx_ctime)));
        }

        content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
            TtMtime, timestampToNano(this->m_st.stx_mtime)));

        content.encode();
        return content;
    }

    void decode(const ndn::Block &content) {
        content.parse();

        this->m_versionedName.wireDecode(content.get(ndn::tlv::Name));

        auto finalBlockId = content.get(ndn::tlv::FinalBlockId);
        this->m_lastSegment =
            ndn::Name::Component(finalBlockId.blockFromValue()).toSegment();

        this->m_segmentSize =
            ndn::readNonNegativeInteger(content.get(TtSegmentSize));

        this->m_st.stx_size = ndn::readNonNegativeInteger(content.get(TtSize));
        this->m_st.stx_mode = ndn::readNonNegativeInteger(content.get(TtMode));

        if (content.find(TtAtime) != content.elements_end()) {
            this->m_st.stx_atime = nanoToTimestamp(
                ndn::readNonNegativeInteger(content.get(TtAtime)));
        } else {
            this->m_st.stx_atime = nanoToTimestamp(0);
        }

        if (content.find(TtBtime) != content.elements_end()) {
            this->m_st.stx_btime = nanoToTimestamp(
                ndn::readNonNegativeInteger(content.get(TtBtime)));
        } else {
            this->m_st.stx_btime = nanoToTimestamp(0);
        }

        if (content.find(TtCtime) != content.elements_end()) {
            this->m_st.stx_ctime = nanoToTimestamp(
                ndn::readNonNegativeInteger(content.get(TtCtime)));
        } else {
            this->m_st.stx_ctime = nanoToTimestamp(0);
        }

        this->m_st.stx_mtime =
            nanoToTimestamp(ndn::readNonNegativeInteger(content.get(TtMtime)));
    }

  private:
    bool has(uint32_t bit) const { return (this->m_st.stx_mask & bit) == bit; }

    uint64_t timestampToNano(struct statx_timestamp t) const {
        return static_cast<uint64_t>(t.tv_sec) * 1000000000 + t.tv_nsec;
    }

    struct statx_timestamp nanoToTimestamp(uint64_t n) const {
        struct statx_timestamp t;
        t.tv_sec = static_cast<int64_t>((n - n % 1000000000) / 1000000000);
        t.tv_nsec = static_cast<uint32_t>(n % 1000000000);
        return t;
    }

  public:
    ndn::Name getVersionedName() { return this->m_versionedName; }
    uint64_t getLastSegment() { return this->m_lastSegment; }
    uint64_t getSegmentSize() { return this->m_segmentSize; }
    uint64_t getFileSize() { return this->m_st.stx_size; }
    uint64_t getMode() { return this->m_st.stx_mode; }

    struct statx_timestamp getAtime() {
        return this->m_st.stx_atime;
    }

    struct statx_timestamp getBtime() {
        return this->m_st.stx_btime;
    }

    struct statx_timestamp getCtime() {
        return this->m_st.stx_ctime;
    }

    struct statx_timestamp getMtime() {
        return this->m_st.stx_mtime;
    }

    uint64_t getAtimeAsInt() { return timestampToNano(this->m_st.stx_atime); }
    uint64_t getBtimeAsInt() { return timestampToNano(this->m_st.stx_btime); }
    uint64_t getCtimeAsInt() { return timestampToNano(this->m_st.stx_ctime); }
    uint64_t getMtimeAsInt() { return timestampToNano(this->m_st.stx_mtime); }

  private:
    ndn::Name m_versionedName;
    uint64_t m_lastSegment;
    uint64_t m_segmentSize;
    struct statx m_st;
};
}; // namespace benchmark
}; // namespace ndnc

#endif // NDNC_APP_BENCHMARK_FT_COMMON_FILE_METADATA_HPP
