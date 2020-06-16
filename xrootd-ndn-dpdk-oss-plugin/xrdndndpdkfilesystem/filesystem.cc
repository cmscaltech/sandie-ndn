/******************************************************************************
 * Named Data Networking plugin for XRootD                                    *
 * Copyright © 2019-2020 California Institute of Technology                   *
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

#include "filesystem.hh"

INIT_ZF_LOG(Xrdndndpdkfilesystem);

using namespace std;

FileSystem::FileSystem()
    : m_FileHandlers{}, m_garbageCollectorInterval(
                            chrono::seconds(FILESYSTEM_GB_INTERVAL_DEFAULT)) {
    this->startGarbageCollector();
}

FileSystem::~FileSystem() {
    this->stopGarbageCollector();

    unique_lock<shared_timed_mutex> lock(m_protectFileHandlers);
    m_FileHandlers.clear();
}

int FileSystem::storeFileHandler(const char *pathname, std::any fd) {
    ZF_LOGD("Store file descriptor for file: %s", pathname);

    unique_lock<shared_timed_mutex> lock(m_protectFileHandlers);
    if (m_FileHandlers.find(pathname) != m_FileHandlers.end()) {
        ZF_LOGW("Trying to replace file descriptor map entry for file: %s",
                pathname);
        return FILESYSTEM_EFAILURE;
    }

    m_FileHandlers[pathname] = FileHandler(fd);
    return FILESYSTEM_ESUCCESS;
}

std::any FileSystem::getFileHandler(const char *pathname) {
    ZF_LOGD("Get file handler for file: %s", pathname);

    shared_lock<shared_timed_mutex> lock(m_protectFileHandlers);
    if (m_FileHandlers.find(pathname) == m_FileHandlers.end()) {
        ZF_LOGE("Unavailable file descriptor for file: %s", pathname);
        return std::any();
    }

    return m_FileHandlers[pathname].getFileDescriptor();
}

bool FileSystem::hasFileHandler(const char *pathname) {
    shared_lock<shared_timed_mutex> lock(m_protectFileHandlers);
    return m_FileHandlers.find(pathname) != m_FileHandlers.end();
}

void FileSystem::startGarbageCollector() {
    ZF_LOGV("Starting file descriptors garbage collector thread");
    m_garbageCollectorThread = thread(&FileSystem::onGarbageCollector, this);
}

void FileSystem::stopGarbageCollector() {
    ZF_LOGV("Stopping file descriptors garbage collector thread");

    m_garbageCollectorStop = true;
    m_garbageCollectorTimerCv.notify_all();

    if (m_garbageCollectorThread.joinable())
        m_garbageCollectorThread.join();
}

void FileSystem::onGarbageCollector() {
    while (true) {
        if (m_garbageCollectorStop)
            return;

        {
            unique_lock<mutex> lock(m_garbageCollectorTimerMtx);
            if (m_garbageCollectorTimerCv.wait_until(
                    lock,
                    std::chrono::steady_clock::now() +
                        m_garbageCollectorInterval,
                    [&]() { return m_garbageCollectorStop != false; }))
                return;
        }

        unique_lock<shared_timed_mutex> lock(m_protectFileHandlers);

        for (auto it = m_FileHandlers.begin(); it != m_FileHandlers.end();) {
            auto timeSinceLastAccessed = std::chrono::steady_clock::now() -
                                         it->second.getOpenTimestamp();

            if (timeSinceLastAccessed > m_garbageCollectorInterval) {
                ZF_LOGI("Closing file descriptor for file: %s",
                        it->first.c_str());
                this->close(it->first.c_str(), it->second.getFileDescriptor());
                it = m_FileHandlers.erase(it);
            } else {
                ++it;
            }
        }
        lock.unlock();
    }
}
