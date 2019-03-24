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

#include "../common/xrdndn-logger.hh"
#include "../common/xrdndn-namespace.hh"
#include "../common/xrdndn-utils.hh"

#include "xrdndn-interest-manager.hh"

using namespace ndn;

namespace xrdndnproducer {
const size_t InterestManager::NUM_TH_INTEREST_HANDLER = 16;
const std::chrono::seconds InterestManager::GARBAGE_COLLECTOR_TIMER =
    std::chrono::seconds(100);                                        // 10 min
const int64_t InterestManager::GARBAGE_COLLECTOR_TIME_DURATION = 300; // 5 min

InterestManager::InterestManager(onDataCallback dataCallback)
    : m_ioServiceWork(m_ioService) {
    m_onDataCallback = std::move(dataCallback);
    m_packager = std::make_shared<Packager>();

    for (size_t i = 0; i < NUM_TH_INTEREST_HANDLER; ++i) {
        m_threads.create_thread(
            std::bind(static_cast<size_t (boost::asio::io_service::*)()>(
                          &boost::asio::io_service::run),
                      &m_ioService));
    }

    m_garbageCollectorTimer =
        std::make_shared<boost::asio::system_timer>(m_ioService);
    m_garbageCollectorTimer->expires_from_now(GARBAGE_COLLECTOR_TIMER);
    m_garbageCollectorTimer->async_wait(
        std::bind(&InterestManager::onGarbageCollector, this));
}

InterestManager::~InterestManager() {
    m_garbageCollectorTimer->cancel();
    m_FileHandlers.clear();

    m_ioService.stop();
    m_threads.join_all();
}

std::shared_ptr<FileHandler> InterestManager::getFileHandler(std::string path) {
    auto it = m_FileHandlers.find(path);
    if (it != m_FileHandlers.end()) {
        return it->second->shared_from_this();
    }

    auto fh = FileHandler::getFileHandler(path, m_packager->shared_from_this());
    if (!fh) {
        NDN_LOG_WARN("Unable to get FileHandler object for file: " << path);
        return std::shared_ptr<FileHandler>(nullptr);
    }

    boost::unique_lock<boost::shared_mutex> lock(m_FileHandlersMtx);
    m_FileHandlers[path] = fh;

    return m_FileHandlers[path]->shared_from_this();
}

void InterestManager::onGarbageCollector() {
    NDN_LOG_TRACE("onGarbageCollector");

    boost::unique_lock<boost::shared_mutex> lock(m_FileHandlersMtx);
    auto gbt = boost::posix_time::second_clock::local_time();

    for (auto it = m_FileHandlers.begin(); it != m_FileHandlers.end();) {
        auto tdiff = (gbt - it->second->getAccessTime()).total_seconds();

        if (tdiff > GARBAGE_COLLECTOR_TIME_DURATION) {
            NDN_LOG_INFO("Garbage collector will erase map entry for file: "
                         << it->first);
            it = m_FileHandlers.erase(it);
        } else
            ++it;
    }

    m_garbageCollectorTimer->expires_from_now(GARBAGE_COLLECTOR_TIMER);
    m_garbageCollectorTimer->async_wait(
        std::bind(&InterestManager::onGarbageCollector, this));
}

void InterestManager::openInterest(const Interest &interest) {
    m_ioService.post([&] {
        Name name = interest.getName();
        ndn::Data data;

        auto fh = getFileHandler(xrdndn::Utils::getPath(name));
        data = fh ? fh->getOpenData(name)
                  : m_packager->getPackage(name, XRDNDN_EFAILURE);

        m_onDataCallback(data);
    });
}

void InterestManager::closeInterest(const Interest &interest) {
    m_ioService.post([&] {
        Name name = interest.getName();
        ndn::Data data;

        auto fh = getFileHandler(xrdndn::Utils::getPath(name));
        data = fh ? fh->getCloseData(name)
                  : m_packager->getPackage(name, XRDNDN_EFAILURE);

        m_onDataCallback(data);
    });
}

void InterestManager::fstatInterest(const Interest &interest) {
    m_ioService.post([&] {
        Name name = interest.getName();
        ndn::Data data;

        auto fh = getFileHandler(xrdndn::Utils::getPath(name));
        data = fh ? fh->getFstatData(name)
                  : m_packager->getPackage(name, XRDNDN_EFAILURE);

        m_onDataCallback(data);
    });
}

void InterestManager::readInterest(const Interest &interest) {
    m_ioService.post([&] {
        Name name = interest.getName();
        ndn::Data data;

        auto fh = getFileHandler(xrdndn::Utils::getPath(name));
        data = fh ? fh->getReadData(name)
                  : m_packager->getPackage(name, XRDNDN_EFAILURE);

        m_onDataCallback(data);
    });
}
} // namespace xrdndnproducer