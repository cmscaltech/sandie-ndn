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

#ifndef XRDNDN_FILE_HANDLER
#define XRDNDN_FILE_HANDLER

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>

#include "xrdndn-dfh-interface.hh"
#include "xrdndn-lru-cache.hh"
#include "xrdndn-packager.hh"

using namespace ndn;

namespace xrdndnproducer {
static const ndn::time::milliseconds CLOSING_FILE_DELAY =
    ndn::time::seconds(180);

class FileDescriptor {
  public:
    FileDescriptor(const char *filePath);
    ~FileDescriptor();

    int get();
    bool isOpened();

  private:
    int m_fd;
};

class FileHandler : xrdndn::FileHandlerInterface {
    const size_t NUM_THREADS_PER_FILEHANDLER = 2;
    const size_t CACHE_SZ = 9472;     // 66304 KB
    const size_t CACHE_LINE_SZ = 148; // 1036 KB

  public:
    FileHandler();
    ~FileHandler();

    std::shared_ptr<Data> getOpenData(const ndn::Interest &interest);
    std::shared_ptr<Data> getCloseData(const ndn::Interest &interest);
    std::shared_ptr<Data> getFStatData(const ndn::Interest &interest);
    std::shared_ptr<Data> getReadData(const ndn::Interest &interest);

  private:
    virtual int Open(std::string path) override;
    virtual int Close(std::string path) override;
    virtual int Fstat(struct stat *buff, std::string path) override;
    ssize_t Read(void *, off_t, size_t, std::string) override { return 0; }
    ssize_t Read(void *buff, size_t count, off_t offset);
    void readCacheLine(off_t offset);
    void insertEmptyCacheLine(off_t offset);
    void waitForPackage(off_t segmentNo);

  private:
    boost::asio::io_service m_ioService;
    boost::asio::io_service::work m_ioServiceWork;
    boost::thread_group m_threads;

    ndn::util::Scheduler m_scheduler;
    ndn::util::scheduler::EventId m_FileClosingEvent;

    std::string m_FilePath;
    std::shared_ptr<Packager> m_packager;
    std::shared_ptr<FileDescriptor> m_FileDescriptor;
    std::shared_ptr<LRUCache<uint64_t, ndn::Data>> m_LRUCache;
    boost::mutex mtx_FileReader;
};
} // namespace xrdndnproducer

#endif // XRDNDN_FILE_HANDLER