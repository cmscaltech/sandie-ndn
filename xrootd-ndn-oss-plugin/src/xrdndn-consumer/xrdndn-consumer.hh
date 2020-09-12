// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef XRDNDN_CONSUMER_HH
#define XRDNDN_CONSUMER_HH

#include <atomic>
#include <map>
#include <sys/stat.h>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/v2/validation-error.hpp>
#include <ndn-cxx/security/v2/validator.hpp>
#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/time.hpp>

#include <boost/noncopyable.hpp>

#include "../common/xrdndn-namespace.hh"
#include "xrdndn-consumer-options.hh"
#include "xrdndn-pipeline.hh"

namespace xrdndnconsumer {
/**
 * @brief This is the multi-threaded NDN Consumer for XRootD NDN OSS plug-in.
 * One instance per file.
 *
 * It translates file system calls into Interest packets and expresess them over
 * the NDN network. It takes care of each individual Interest and will return
 * specific Data for each system call
 *
 */
class Consumer : public std::enable_shared_from_this<Consumer>,
                 private boost::noncopyable {
    using DataTypeTuple = std::tuple<int, ndn::Interest, ndn::Data>;
    using FutureType = std::future<DataTypeTuple>;

  public:
    /**
     * @brief Returns a pointer to a Consumer object instance.
     *
     * @param opts Consumer instance options
     * @return std::shared_ptr<Consumer> If the Consumer is not able to connect
     * to local forwarder it will return nullptr, else the Consumer instance
     * will be returned
     */

    static std::shared_ptr<Consumer>
    getXrdNdnConsumerInstance(const Options &opts = Options());

    /**
     * @brief Construct a new Consumer object
     *
     * @param opts Consumer instance options
     */
    Consumer(const Options &opts);

    /**
     * @brief Destroy the Consumer object
     *
     */
    ~Consumer();

    /**
     * @brief Open file function over NDN. Convert to a corresponding Interest
     * packet
     *
     * @param path File path of the file to be copied over NDN network
     * @return int The return value of open POSIX system call on the Producer
     * side. 0 (success) / -errno (error)
     */
    int Open(std::string path);

    /**
     * @brief Dummy close function. File closing is handled on Producer
     *
     * @return int 0 (success)
     */
    int Close();

    /**
     * @brief Fstat file function over NDN. Convert to a corresponding Interest
     * packet
     *
     * @param buff If fstat is possible, the POSIX struct stat of file will be
     * put in it
     * @return int The return value of fstat POSIX system call on the Producer
     * side. 0 (success and buff will have data) / -errno (error)
     */
    int Fstat(struct stat *buff);

    /**
     * @brief Read blen bytes from file over NDN. The request will be translated
     * into a number of Interest packets
     *
     * @param buff The address where data will be stored
     * @param offset Offset in file were the read will begin
     * @param blen The number of bytes to be read by Producer
     * @return ssize_t On Success the actual number of bytes read on the
     * Producer side. On failure -errno
     */
    ssize_t Read(void *buff, off_t offset, size_t blen);

  private:
    /**
     * @brief Set the Log Level object
     *
     */
    void setLogLevel();

    /**
     * @brief Process Interests
     *
     * @param keepThread Keep thread in a blocked state (in event processing),
     * even when there are no outstanding events (e.g., no Interest/Data is
     * expected)
     */
    void processEvents(bool keepThread = true);

    /**
     * @brief Create Interest packet
     *
     * @param prefix ndn::Name prefix of Interest Name
     * @param segmentNo Segment number of Interest packet
     * @return const ndn::Interest The resulting Interest packet
     */
    const ndn::Interest getInterest(ndn::Name prefix, uint64_t segmentNo = 0);

    /**
     * @brief If errcode is success 0, means Data was acquired for Interest.
     * Validates Data. In case of success it returns errorcode set by Producer
     *
     * @param errcode The result of fetching Data set by DataFetcher
     * @param interest The Interest packet
     * @param data Data for the Interest packet. If errcode is not 0, this is
     * empty
     * @return int If errcode is not 0 (success), it returns the errcode (the
     * results of expressing Interest by DataFetcher). -1 If Data is not valid.
     * The errorcode set by Producer for the specific Interest operation
     */
    int validateData(const int errcode, const ndn::Interest &interest,
                     const ndn::Data &data);

    /**
     * @brief Put data in the provided buffer from dataStore
     *
     * @param buff The buffer
     * @param offset The offset in content where blen bytes were requested
     * @param blen The buffer size
     * @param dataStore Ordered map of all Data received for a read request
     * @return off_t The actual number of bytes that have been put in the buffer
     */

    size_t returnData(void *buff, off_t offset, size_t blen,
                      std::map<uint64_t, const ndn::Block> &dataStore);

  private:
    const Options m_options;
    ndn::time::seconds m_interestLifetime;
    std::string m_path;

    ndn::Face m_face;
    ndn::security::v2::Validator &m_validator;

    boost::thread faceProcessEventsThread;

    std::atomic<bool> m_error;
    std::shared_ptr<Pipeline> m_pipeline;
};
} // namespace xrdndnconsumer

#endif // XRDNDN_CONSUMER_HH