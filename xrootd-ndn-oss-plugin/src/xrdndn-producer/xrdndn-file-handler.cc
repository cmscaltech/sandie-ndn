// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <array>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../common/xrdndn-logger.hh"
#include "../common/xrdndn-utils.hh"
#include "xrdndn-file-handler.hh"

using namespace ndn;

namespace xrdndnproducer {
std::shared_ptr<FileHandler>
FileHandler::getFileHandler(const std::string path,
                            const std::shared_ptr<Packager> &packager) {
    auto fh = std::make_shared<FileHandler>(path, packager);
    return fh;
}

FileHandler::FileHandler(const std::string path,
                         const std::shared_ptr<Packager> &packager)
    : m_fd(XRDNDN_EFAILURE), m_path(path), m_packager(packager) {
    accessTime = boost::posix_time::second_clock::local_time();
    Open();
}

FileHandler::~FileHandler() {
    NDN_LOG_INFO("Dealloc FileHandler object for file: " << m_path);

    if (m_fd != XRDNDN_EFAILURE) {
        close(m_fd);
    }
}

boost::posix_time::ptime FileHandler::getAccessTime() { return accessTime; }

/*****************************************************************************/
/*                                  O p e n                                  */
/*****************************************************************************/
std::shared_ptr<ndn::Data> FileHandler::getOpenData(ndn::Name &name) {
    accessTime = boost::posix_time::second_clock::local_time();

    auto retOpen = Open();
    return m_packager->getPackage(name, retOpen);
}

int FileHandler::Open() {
    if (isOpened()) {
        NDN_LOG_INFO("File: " << m_path << " already opened");
        return XRDNDN_ESUCCESS;
    }

    NDN_LOG_INFO("Open file: " << m_path);

    if ((m_fd = open(m_path.c_str(), O_RDONLY)) == XRDNDN_EFAILURE) {
        NDN_LOG_WARN("Failed to open file: " << m_path << ": "
                                             << strerror(errno));
        return -errno;
    }

    return XRDNDN_ESUCCESS;
}

bool FileHandler::isOpened() { return m_fd == XRDNDN_EFAILURE ? false : true; }

/*****************************************************************************/
/*                                F s t a t                                  */
/*****************************************************************************/
std::shared_ptr<ndn::Data> FileHandler::getFstatData(ndn::Name &name) {
    accessTime = boost::posix_time::second_clock::local_time();

    std::array<uint8_t, sizeof(struct stat)> info;
    int retFstat = Fstat(&info);

    if (retFstat != XRDNDN_ESUCCESS) {
        return m_packager->getPackage(name, retFstat);
    } else {
        return m_packager->getPackage(name, info.data(), info.size());
    }
}

int FileHandler::Fstat(void *buff) {
    NDN_LOG_INFO("Fstat file: " << m_path);

    if (stat(m_path.c_str(), static_cast<struct stat *>(buff)) ==
        XRDNDN_EFAILURE) {
        NDN_LOG_WARN("Failed to fstat file: " << m_path << ": "
                                              << strerror(errno));
        return -errno;
    }

    return XRDNDN_ESUCCESS;
}

/*****************************************************************************/
/*                                  R e a d                                  */
/*****************************************************************************/
std::shared_ptr<ndn::Data> FileHandler::getReadData(ndn::Name &name) {
    accessTime = boost::posix_time::second_clock::local_time();

    std::array<uint8_t, XRDNDN_MAX_NDN_PACKET_SIZE> blockFromFile;
    auto retRead =
        Read(&blockFromFile, XRDNDN_MAX_NDN_PACKET_SIZE,
             xrdndn::Utils::getSegmentNo(name) * XRDNDN_MAX_NDN_PACKET_SIZE);

    if (retRead < 0) {
        return m_packager->getPackage(name, retRead);
    } else {
        return m_packager->getPackage(name, blockFromFile.data(), retRead);
    }
}

ssize_t FileHandler::Read(void *buff, size_t count, off_t offset) {
    auto ret = pread(m_fd, buff, count, offset);
    if (ret == XRDNDN_EFAILURE) {
        NDN_LOG_WARN("Failed to read " << count << " bytes" << m_path << " @"
                                       << offset << " from file: " << m_path
                                       << ": " << strerror(errno));
        return -errno;
    }

    return ret;
}
} // namespace xrdndnproducer
