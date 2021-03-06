// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "../common/xrdndn-logger.hh"
#include "../common/xrdndn-namespace.hh"
#include "../common/xrdndn-utils.hh"

#include "xrdndn-interest-manager.hh"

using namespace ndn;

namespace xrdndnproducer {
InterestManager::InterestManager(const Options &opts,
                                 onDataCallback dataCallback)
    : m_ioServiceWork(m_ioService), m_options(opts) {
    m_onDataCallback = std::move(dataCallback);
    m_packager = std::make_shared<Packager>(m_options.freshnessPeriod,
                                            m_options.disableSigning);

    for (size_t i = 0; i < m_options.nthreads; ++i) {
        m_threads.create_thread(
            std::bind(static_cast<size_t (boost::asio::io_service::*)()>(
                          &boost::asio::io_service::run),
                      &m_ioService));
    }

    m_garbageCollectorTimer =
        std::make_shared<boost::asio::system_timer>(m_ioService);
    m_garbageCollectorTimer->expires_from_now(m_options.gbTimer);
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
    boost::unique_lock<boost::shared_mutex> lock(m_FileHandlersMtx);

    auto it = m_FileHandlers.find(path);
    if (it != m_FileHandlers.end()) {
        return it->second->shared_from_this();
    }

    auto fh = FileHandler::getFileHandler(path, m_packager->shared_from_this());
    if (!fh) {
        NDN_LOG_WARN("Unable to get FileHandler object for file: " << path);
        return std::shared_ptr<FileHandler>(nullptr);
    }

    m_FileHandlers[path] = fh;
    return m_FileHandlers[path]->shared_from_this();
}

void InterestManager::onGarbageCollector() {
    NDN_LOG_TRACE("onGarbageCollector");

    boost::unique_lock<boost::shared_mutex> lock(m_FileHandlersMtx);
    auto gbt = boost::posix_time::second_clock::local_time();

    for (auto it = m_FileHandlers.begin(); it != m_FileHandlers.end();) {
        auto tdiff = (gbt - it->second->getAccessTime()).total_seconds();

        if (tdiff > m_options.gbFileLifeTime) {
            NDN_LOG_INFO("Garbage collector will erase map entry for file: "
                         << it->first);
            it = m_FileHandlers.erase(it);
        } else
            ++it;
    }

    m_garbageCollectorTimer->expires_from_now(m_options.gbTimer);
    m_garbageCollectorTimer->async_wait(
        std::bind(&InterestManager::onGarbageCollector, this));
}

void InterestManager::openInterest(const Interest &interest) {
    m_ioService.post([&] {
        Name name = interest.getName();

        std::string path = xrdndn::Utils::getPath(name);
        if (path.empty())
            return;

        auto fh = getFileHandler(path);
        auto data = fh ? fh->getOpenData(name)
                       : m_packager->getPackage(name, XRDNDN_EFAILURE);

        m_onDataCallback(data);
    });
}

void InterestManager::fstatInterest(const Interest &interest) {
    m_ioService.post([&] {
        Name name = interest.getName();

        std::string path = xrdndn::Utils::getPath(name);
        if (path.empty())
            return;

        auto fh = getFileHandler(path);
        auto data = fh ? fh->getFstatData(name)
                       : m_packager->getPackage(name, XRDNDN_EFAILURE);

        m_onDataCallback(data);
    });
}

void InterestManager::readInterest(const Interest &interest) {
    m_ioService.post([&] {
        Name name = interest.getName();

        std::string path = xrdndn::Utils::getPath(name);
        if (path.empty())
            return;

        auto fh = getFileHandler(path);
        auto data = fh ? fh->getReadData(name)
                       : m_packager->getPackage(name, XRDNDN_EFAILURE);

        m_onDataCallback(data);
    });
}
} // namespace xrdndnproducer