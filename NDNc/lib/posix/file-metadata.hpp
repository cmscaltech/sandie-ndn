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

#ifndef NDNC_LIB_POSIX_FILE_METADATA_HPP
#define NDNC_LIB_POSIX_FILE_METADATA_HPP

#include <cstdint>
#include <math.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include "file-rdr.hpp"

namespace ndnc::posix {
static const uint32_t STATX_REQUIRED =
    STATX_TYPE | STATX_MODE | STATX_MTIME | STATX_SIZE;
static const uint32_t STATX_OPTIONAL = STATX_ATIME | STATX_CTIME | STATX_BTIME;

/**
 * @brief
 * https://github.com/yoursunny/ndn6-tools/blob/main/file-server.md#protocol-details
 *
 */
class FileMetadata {
    enum {
        TtSegmentSize = 0xF500,
        TtSize = 0xF502,
        TtMode = 0xF504, // always present
        TtAtime = 0xF506,
        TtBtime = 0xF508,
        TtCtime = 0xF50A,
        TtMtime = 0xF50C, // always present
    };

  public:
    FileMetadata() {
    }

    FileMetadata(uint64_t segmentSize) : segmentSize_{segmentSize} {
        this->segmentSize_ = segmentSize;
    }

    FileMetadata(const ndn::Block &content) {
        decode(content);
    }

    ~FileMetadata() {
    }

    bool prepare(const std::string path, ndn::Name name) {
        int res = syscall(__NR_statx, -1, path.c_str(), 0,
                          STATX_REQUIRED | STATX_OPTIONAL, &this->stx_);

        if (res != 0 || !has(STATX_REQUIRED)) {
            return false;
        }

        this->versionedName_ =
            name.appendVersion(timestamp_to_uint64(stx_.stx_mtime));

        this->finalBlockId_ =
            (uint64_t)(ceil((double)stx_.stx_size / segmentSize_));

        return true;
    }

    ndn::Block encode() {
        ndn::Block content(ndn::tlv::Content);

        // Name: versioned name prefix; version number is derived from last
        // modification time
        content.push_back(this->versionedName_.wireEncode());

        // FinalBlockId: enclosed SegmentNameComponent that reflects
        // last segment number (inclusive)
        ndn::Block finalBlockId{ndn::tlv::FinalBlockId};
        finalBlockId.push_back(
            ndn::name::Component::fromSegment(this->finalBlockId_)
                .wireEncode());
        finalBlockId.encode();
        content.push_back(finalBlockId);

        // SegmentSize (TLV-TYPE 0xF500, NonNegativeInteger): segment
        // size (octets); last segment may be shorter
        content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
            TtSegmentSize, this->segmentSize_));

        // Size (TLV-TYPE 0xF502, NonNegativeInteger): file size
        // (octets)
        content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
            TtSize, this->stx_.stx_size));

        // Mode (TLV-TYPE 0xF504, NonNegativeInteger): file type and mode,
        // see https://man7.org/linux/man-pages/man7/inode.7.html
        content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
            TtMode, this->stx_.stx_mode));

        // atime (TLV-TYPE 0xF506, NonNegativeInteger): last accessed time
        // (nanoseconds since Unix epoch)
        if (has(STATX_ATIME)) {
            content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
                TtAtime, timestamp_to_uint64(this->stx_.stx_atime)));
        }

        // btime (TLV-TYPE 0xF508, NonNegativeInteger): creation time
        // (nanoseconds since Unix epoch)
        if (has(STATX_BTIME)) {
            content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
                TtBtime, timestamp_to_uint64(this->stx_.stx_btime)));
        }

        // ctime (TLV-TYPE 0xF50A, NonNegativeInteger): last status change
        // time (nanoseconds since Unix epoch)
        if (has(STATX_CTIME)) {
            content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
                TtCtime, timestamp_to_uint64(this->stx_.stx_ctime)));
        }

        // mtime (TLV-TYPE 0xF50C, NonNegativeInteger): last modification
        // time (nanoseconds since Unix epoch)
        content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(
            TtMtime, timestamp_to_uint64(this->stx_.stx_mtime)));

        content.encode();
        return content;
    }

    void decode(const ndn::Block &content) {
        content.parse();

        this->versionedName_.wireDecode(content.get(ndn::tlv::Name));

        if (content.find(ndn::tlv::FinalBlockId) != content.elements_end()) {
            auto finalBlockId = content.get(ndn::tlv::FinalBlockId);
            this->finalBlockId_ =
                ndn::Name::Component(finalBlockId.blockFromValue()).toSegment();
        } else {
            this->finalBlockId_ = 0;
        }

        if (content.find(TtSegmentSize) != content.elements_end()) {
            this->segmentSize_ =
                ndn::readNonNegativeInteger(content.get(TtSegmentSize));
        } else {
            this->segmentSize_ = 0;
        }

        if (content.find(TtSize) != content.elements_end()) {
            this->stx_.stx_size =
                ndn::readNonNegativeInteger(content.get(TtSize));
        } else {
            this->stx_.stx_size = 0;
        }

        this->stx_.stx_mode = ndn::readNonNegativeInteger(content.get(TtMode));

        if (content.find(TtAtime) != content.elements_end()) {
            this->stx_.stx_atime = uint64_to_timestamp(
                ndn::readNonNegativeInteger(content.get(TtAtime)));
        } else {
            this->stx_.stx_atime = uint64_to_timestamp(0);
        }

        if (content.find(TtBtime) != content.elements_end()) {
            this->stx_.stx_btime = uint64_to_timestamp(
                ndn::readNonNegativeInteger(content.get(TtBtime)));
        } else {
            this->stx_.stx_btime = uint64_to_timestamp(0);
        }

        if (content.find(TtCtime) != content.elements_end()) {
            this->stx_.stx_ctime = uint64_to_timestamp(
                ndn::readNonNegativeInteger(content.get(TtCtime)));
        } else {
            this->stx_.stx_ctime = uint64_to_timestamp(0);
        }

        this->stx_.stx_mtime = uint64_to_timestamp(
            ndn::readNonNegativeInteger(content.get(TtMtime)));
    }

  private:
    bool has(uint32_t bit) const {
        return (this->stx_.stx_mask & bit) == bit;
    }

    uint64_t timestamp_to_uint64(struct statx_timestamp t) const {
        return static_cast<uint64_t>(t.tv_sec) * 1000000000 + t.tv_nsec;
    }

    struct statx_timestamp uint64_to_timestamp(uint64_t n) const {
        struct statx_timestamp t;
        t.tv_sec = static_cast<int64_t>((n - n % 1000000000) / 1000000000);
        t.tv_nsec = static_cast<uint32_t>(n % 1000000000);
        return t;
    }

    void copy_statx_to_stat_time(struct statx_timestamp *stx,
                                 struct timespec *st) {
        st->tv_sec = stx->tv_sec;
        st->tv_nsec = stx->tv_nsec;
    }

  public:
    ndn::Name getVersionedName() {
        return this->versionedName_;
    }

    uint64_t getFinalBlockID() {
        return this->finalBlockId_;
    }

    uint64_t getSegmentSize() {
        return this->segmentSize_;
    }

    uint64_t getFileSize() {
        return this->stx_.stx_size;
    }

    struct statx_timestamp getAtime() {
        return this->stx_.stx_atime;
    }

    struct statx_timestamp getBtime() {
        return this->stx_.stx_btime;
    }

    struct statx_timestamp getCtime() {
        return this->stx_.stx_ctime;
    }

    struct statx_timestamp getMtime() {
        return this->stx_.stx_mtime;
    }

    bool isFile() {
        return S_ISREG(stx_.stx_mode);
    }

    bool isDir() {
        return S_ISDIR(stx_.stx_mode);
    }

    void fstat(struct stat *st) {
        if (st == nullptr) {
            return;
        }

        st->st_mode = stx_.stx_mode;
        st->st_size = stx_.stx_size;
        copy_statx_to_stat_time(&stx_.stx_atime, &st->st_atim);
        copy_statx_to_stat_time(&stx_.stx_ctime, &st->st_ctim);
        copy_statx_to_stat_time(&stx_.stx_mtime, &st->st_mtim);
    }

    std::string timestamp_to_string(struct statx_timestamp t) {
        return "tv_sec(" + std::to_string(t.tv_sec) + ") tv_nsec(" +
               std::to_string(t.tv_nsec) + ")";
    }

  private:
    ndn::Name versionedName_;

    uint64_t finalBlockId_;
    uint64_t segmentSize_;

    struct statx stx_;
};
}; // namespace ndnc::posix

#endif // NDNC_LIB_POSIX_FILE_METADATA_HPP
