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

#pragma once

#ifndef XRDNDNDPDK_FILESYSTEM_HH
#define XRDNDNDPDK_FILESYSTEM_HH

#include <mutex>
#include <shared_mutex>
#include <stdio.h>
#include <sys/types.h>
#include <thread>
#include <unordered_map>

#include "../xrdndndpdk-logger/logger.hh"
#include "filesystem-namespace.hh"

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

struct FileHandler {
    FileHandler() : fd(0) {}
    FileHandler(int fd) : fd(fd) {}

    FileHandler &operator=(const FileHandler &f) {
        if (this != &f) {
            std::unique_lock<std::shared_timed_mutex> lhs_lock(mtx,
                                                               std::defer_lock);
            std::shared_lock<std::shared_timed_mutex> rhs_lock(f.mtx,
                                                               std::defer_lock);
            std::lock(lhs_lock, rhs_lock);
            fd = f.fd;
            openTimestamp = f.openTimestamp;
        }

        return *this;
    }

    decltype(auto) getOpenTimestamp() { return openTimestamp; }
    int getFileDescriptor() { return fd; }

  private:
    int fd;
    std::chrono::steady_clock::time_point openTimestamp;
    mutable std::shared_timed_mutex mtx;
};

class FileSystem {
  public:
    FileSystem();
    FileSystem(uint32_t gbInterval);
    virtual ~FileSystem() = 0;

    virtual int open(const char *pathname) = 0;
    virtual int fstat(const char *pathname, void *buf) = 0;
    virtual int read(const char *pathname, void *buf, size_t count = 0,
                     off_t offset = 0) = 0;
    virtual void close(const char *pathname, int fd) = 0;

  protected:
    int storeFileHandler(const char *pathname, int fd);
    int getFileHandler(const char *pathname);
    bool hasFileHandler(const char *pathname);

  private:
    void startGarbageCollector();
    void stopGarbageCollector();
    void onGarbageCollector();

  private:
    std::unordered_map<std::string, FileHandler> m_FileHandlers;
    std::shared_timed_mutex m_protectFileHandlers;

    std::thread m_garbageCollectorThread;
    std::chrono::seconds m_garbageCollectorInterval;

    std::condition_variable m_garbageCollectorTimerCv;
    std::mutex m_garbageCollectorTimerMtx;
    bool m_garbageCollectorStop = false;
};

#endif // XRDNDNDPDK_FILESYSTEM_HH