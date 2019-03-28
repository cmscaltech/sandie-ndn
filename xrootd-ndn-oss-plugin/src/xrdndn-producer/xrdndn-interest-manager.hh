/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2019 California Institute of Technology                        *
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

#ifndef XRDNDN_INTEREST_MANAGER_HH
#define XRDNDN_INTEREST_MANAGER_HH

#include <unordered_map>

#include <ndn-cxx/face.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>

#include "xrdndn-file-handler.hh"
#include "xrdndn-producer-options.hh"

namespace xrdndnproducer {

class InterestManager {
    using onDataCallback = std::function<void(const ndn::Data &data)>;

  public:
    InterestManager(const Options &opts, onDataCallback dataCallback);
    ~InterestManager();

    void openInterest(const ndn::Interest &interest);
    void fstatInterest(const ndn::Interest &interest);
    void readInterest(const ndn::Interest &interest);

  private:
    std::shared_ptr<FileHandler> getFileHandler(std::string path);
    void onGarbageCollector();

  private:
    onDataCallback m_onDataCallback;

    boost::asio::io_service m_ioService;
    boost::asio::io_service::work m_ioServiceWork;
    boost::thread_group m_threads;

    std::shared_ptr<Packager> m_packager;
    std::shared_ptr<boost::asio::system_timer> m_garbageCollectorTimer;
    const Options m_options;

    std::unordered_map<std::string, std::shared_ptr<FileHandler>>
        m_FileHandlers;
    mutable boost::shared_mutex m_FileHandlersMtx;
};
} // namespace xrdndnproducer

#endif // XRDNDN_INTEREST_MANAGER_HH