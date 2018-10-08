/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018 California Institute of Technology                        *
 *                                                                            *
 * Author: Catalin Iordache <catalin.iordache@cern.ch>                        *
 *                                                                            *
 * This program is free software: you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.     *
 *****************************************************************************/

#include <array>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

#include "xrdndn-common.hh"
#include "xrdndn-file-handler.hh"
#include "xrdndn-logger.hh"
#include "xrdndn-utils.hh"

namespace xrdndnproducer {
FileDescriptor::FileDescriptor(const char *filePath) {
    m_fd = open(filePath, O_RDONLY);
}

FileDescriptor::~FileDescriptor() {
    if (isOpened()) {
        close(m_fd);
    }
}

int FileDescriptor::get() { return m_fd; }

bool FileDescriptor::isOpened() {
    if (this->m_fd != -1) {
        return true;
    }

    return false;
}

FileHandler::FileHandler()
    : m_ioServiceWork(m_ioService), m_scheduler(m_ioService) {
    for (size_t i = 0; i < NUM_THREADS_PER_FILEHANDLER; ++i) {
        m_threads.create_thread(
            std::bind(static_cast<size_t (boost::asio::io_service::*)()>(
                          &boost::asio::io_service::run),
                      &m_ioService));
    }

    m_LRUCache =
        std::make_shared<LRUCache<uint64_t, Data>>(
            CACHE_SZ, CACHE_LINE_SZ);

    m_packager = std::make_shared<Packager>();
}

FileHandler::~FileHandler() {
    NDN_LOG_TRACE("Dealloc FileHandler for file: ");

    m_scheduler.cancelAllEvents();
    m_ioService.stop();
    m_threads.join_all();
}

/*****************************************************************************/
/*                                  O p e n                                  */
/*****************************************************************************/
std::shared_ptr<Data> FileHandler::getOpenData(const ndn::Interest &interest) {
    Name name(interest.getName());
    int ret = Open(
        xrdndn::Utils::getFilePathFromName(name, xrdndn::SystemCalls::open));
    name.appendVersion();

    return m_packager->getPackage(name, ret);
}

int FileHandler::Open(std::string path) {
    // If the file is already opened, no need to open it again
    if (m_FileDescriptor && m_FileDescriptor->isOpened()) {
        if (m_FileClosingEvent) {
            NDN_LOG_INFO(
                "File already opened. Cancel closing task in progress.");
            m_scheduler.cancelEvent(m_FileClosingEvent);
        }

        return XRDNDN_ESUCCESS;
    }

    m_FileDescriptor = std::make_shared<FileDescriptor>(path.c_str());

    if (!m_FileDescriptor || !m_FileDescriptor->isOpened()) {
        NDN_LOG_WARN("Failed to open file: " << path);
        return XRDNDN_EFAILURE;
    }

    NDN_LOG_INFO("Opened file: " << path);
    m_FilePath = path;
    return XRDNDN_ESUCCESS;
}

/*****************************************************************************/
/*                                 C l o s e                                 */
/*****************************************************************************/
std::shared_ptr<Data> FileHandler::getCloseData(const ndn::Interest &interest) {
    Name name(interest.getName());
    int ret = Close(
        xrdndn::Utils::getFilePathFromName(name, xrdndn::SystemCalls::close));
    name.appendVersion();

    return m_packager->getPackage(name, ret);
}

int FileHandler::Close(std::string path) {
    if (!m_FileDescriptor) {
        NDN_LOG_WARN("File: " << path << " was not oppend previously.");
        return XRDNDN_EFAILURE;
    }

    m_FileClosingEvent =
        m_scheduler.scheduleEvent(CLOSING_FILE_DELAY, [this, path] {
            if (m_FileDescriptor) {
                NDN_LOG_INFO("Closing file: " << path);
            }
        });

    NDN_LOG_INFO("Schedule event for closing file: " << path);
    return XRDNDN_ESUCCESS;
}

/*****************************************************************************/
/*                                F s t a t                                  */
/*****************************************************************************/
std::shared_ptr<Data> FileHandler::getFStatData(const ndn::Interest &interest) {
    Name name(interest.getName());
    struct stat info;
    int ret = Fstat(&info, xrdndn::Utils::getFilePathFromName(
                               name, xrdndn::SystemCalls::fstat));
    name.appendVersion();

    if (ret != 0) {
        return m_packager->getPackage(name, ret);
    }

    // Get stat package
    std::string buff(sizeof(info), '\0');
    memcpy(&buff[0], &info, sizeof(info));

    return m_packager->getPackage(
        name, reinterpret_cast<const uint8_t *>(buff.data()), sizeof(info));
}

int FileHandler::Fstat(struct stat *buff, std::string path) {
    // Call stat as is sufficient
    if (stat(path.c_str(), buff) != 0) {
        NDN_LOG_ERROR("Fstat fail for file: " << path);
        return XRDNDN_EFAILURE;
    }

    NDN_LOG_INFO("Return fstat for file: " << path);
    return XRDNDN_ESUCCESS;
}

/*****************************************************************************/
/*                                  R e a d                                  */
/*****************************************************************************/
// Read chunk from file
ssize_t FileHandler::Read(void *buff, size_t count, off_t offset) {
    if (!m_FileDescriptor || !m_FileDescriptor->isOpened()) {
        NDN_LOG_WARN("Read file: " << m_FilePath
                                   << " was not oppend previously");
        return XRDNDN_EFAILURE;
    }

    auto ret = pread(m_FileDescriptor->get(), buff, count, offset);
    return ret;
}

// Insert a cache line in cache, with empty data entries. This is done in order
// for multiple threads to wait on the same mutex.
void FileHandler::insertEmptyCacheLine(off_t offset) {
    boost::unique_lock<boost::mutex> lock(mtx_FileReader);
    off_t cacheLineEndOffset = offset + CACHE_LINE_SZ;
    for (off_t i = offset; i < cacheLineEndOffset; ++i) {
        auto entry = std::make_shared<LRUCacheEntry<Data>>();
        m_LRUCache->insert(i, entry);
        m_LRUCache->at(i)->avoidEviction = true;
    }
}

// Populate a empty cache line with ndn::Data packages containng chunks form
// file
void FileHandler::readCacheLine(off_t offset) {
    ndn::Name readPrefix =
        xrdndn::Utils::interestName(xrdndn::SystemCalls::read, m_FilePath);
    off_t cacheLineEndOffset = offset + CACHE_LINE_SZ;
    ssize_t len = 0;

    for (off_t i = offset; i < cacheLineEndOffset && len >= 0; ++i) {
        std::array<uint8_t, XRDNDN_MAX_NDN_PACKET_SIZE> blockFromFile;
        len = Read(&blockFromFile, XRDNDN_MAX_NDN_PACKET_SIZE,
                   i * XRDNDN_MAX_NDN_PACKET_SIZE);

        ndn::Name name = readPrefix;
        name.appendSegment(i);
        name.appendVersion();

        std::shared_ptr<ndn::Data> data;
        if (len < 0) {
            data = m_packager->getPackage(name, XRDNDN_EFAILURE);
        } else {
            data = m_packager->getPackage(name, &blockFromFile[0], len);
        }

        // Update previously empty cache line entry and notify waiting thread
        // if (!m_LRUCache->at(i)) {
        //     NDN_LOG_ERROR("entry doesn't exists!!");
        // }
        boost::unique_lock<boost::mutex> lock(m_LRUCache->at(i)->mutex);
        auto entry = m_LRUCache->at(i);
        entry->data = data;
        entry->processingData = true;
        entry->cond.notify_one();
    }
}

// Wait for a worker thread to finish preparing the requested segment from the
// file
void FileHandler::waitForPackage(off_t segmentNo) {
    boost::unique_lock<boost::mutex> lock(m_LRUCache->at(segmentNo)->mutex);
    m_LRUCache->at(segmentNo)->cond.wait(lock, [&] { return m_LRUCache->at(segmentNo)->processingData; });
}

// Return an ndn::Data package containing a chunk from the file
std::shared_ptr<Data> FileHandler::getReadData(const ndn::Interest &interest) {
    std::shared_ptr<Data> ret;
    Name name(interest.getName());

    if (!m_FileDescriptor || !m_FileDescriptor->isOpened()) {
        std::string path =
            xrdndn::Utils::getFilePathFromName(name, xrdndn::SystemCalls::read);

        NDN_LOG_WARN("GetReadData for file: " << path
                                              << " was not opened previosly.");
        if (Open(path) == XRDNDN_EFAILURE) {
            ret = m_packager->getPackage(name, XRDNDN_EFAILURE);
            return ret;
        }
    }

    auto segmentNo = xrdndn::Utils::getSegmentFromPacket(interest);
    if (m_LRUCache->hasKey(segmentNo)) { // Cache HIT
        waitForPackage(segmentNo);
    } else { // Trigger a new task that will populate a cache line and notify
             // when the requested package is ready
        insertEmptyCacheLine(segmentNo);
        m_ioService.post(
            std::bind(&FileHandler::readCacheLine, this, segmentNo));
        waitForPackage(segmentNo);
    }

    ret = m_LRUCache->at(segmentNo)->data;
    m_LRUCache->at(segmentNo)->avoidEviction = false;
    return ret;
}
} // namespace xrdndnproducer
