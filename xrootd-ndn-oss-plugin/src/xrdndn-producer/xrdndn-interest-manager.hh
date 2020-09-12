// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef XRDNDN_INTEREST_MANAGER_HH
#define XRDNDN_INTEREST_MANAGER_HH

#include <unordered_map>

#include <boost/asio/io_service.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>

#include "xrdndn-file-handler.hh"
#include "xrdndn-producer-options.hh"

namespace xrdndnproducer {
/**
 * @brief This class implement the Interest Manager object. All Interest packets
 * received on Face will be passed to this object that will handled them using
 * an asio thread pool
 *
 */
class InterestManager {
    using onDataCallback = std::function<void(std::shared_ptr<ndn::Data> data)>;

  public:
    /**
     * @brief Construct a new Interest Manager object
     *
     * @param opts Producer command line arguments
     * @param dataCallback The callback function to be called for putting Data
     * on Face
     */
    InterestManager(const Options &opts, onDataCallback dataCallback);

    /**
     * @brief Destroy the Interest Manager object
     *
     */
    ~InterestManager();

    /**
     * @brief Handle the Interest for open system call
     *
     * @param interest The Interest received on Face
     */
    void openInterest(const ndn::Interest &interest);

    /**
     * @brief Handle the Interest for fstat system call
     *
     * @param interest The Interest received on Face
     */
    void fstatInterest(const ndn::Interest &interest);

    /**
     * @brief Handle the Interest for read system call
     *
     * @param interest The Interest received on Face
     */
    void readInterest(const ndn::Interest &interest);

  private:
    /**
     * @brief Get FileHandler object for file at path. If the FileHandler does
     * not exist, one will be created, thus the file will be opened for the
     * first time
     *
     * @param path Path to the file for which FileHandler has been requested
     * @return std::shared_ptr<FileHandler> The resulted Filehandler as
     * shared_ptr
     */
    std::shared_ptr<FileHandler> getFileHandler(std::string path);

    /**
     * @brief Function triggered by a timer. Here, all FileHandler objects with
     * accessTime greater than gbFileLifeTime command line argument will be
     * erased, thus the files will be closed and memory freed
     *
     */
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