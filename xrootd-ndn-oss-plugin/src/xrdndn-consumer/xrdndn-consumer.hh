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

#ifndef XRDNDN_CONSUMER_HH
#define XRDNDN_CONSUMER_HH

#include <map>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/v2/validation-error.hpp>
#include <ndn-cxx/security/v2/validator.hpp>
#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/time.hpp>

#include "../common/xrdndn-common.hh"
#include "../common/xrdndn-dfh-interface.hh"
#include "xrdndn-pipeline.hh"

namespace xrdndnconsumer {
/**
 * @brief Struct to keep interest stats of the file beeing processed by this
 * Consumer
 *
 */
struct FileStat {
    uint8_t retOpen = -1;  // Return value of open system call
    uint8_t retClose = -1; // Return value of close system call
    uint8_t retFstat = -1; // Return value of fstat system call

    off_t retRead = 0;  // Return value of read system call
    off_t fileSize = 0; // Size of file beeing processed
};

/**
 * @brief This class implements an NDN Consumer for XRootD NDN OSS plug-in. It
 * translates file system calls into Interest packets and expresess them over
 * the network. It takes care of each individual Interest and will return
 * specific data of each system call
 *
 */
class Consumer : xrdndn::FileHandlerInterface {
    /**
     * @brief The default lifetime of an Interest packet expressed by Consumer
     *
     */
    static const ndn::time::milliseconds DEFAULT_INTEREST_LIFETIME;

  public:
    /**
     * @brief Returns a pointer to a Consumer object instance.
     *
     * @return std::shared_ptr<Consumer> If the Consumer is not able to connect
     * to local forwarder it will return nullptr, else the Consumer instance
     * will be returned
     */
    static std::shared_ptr<Consumer> getXrdNdnConsumerInstance();

    /**
     * @brief Construct a new Consumer object
     *
     */
    Consumer();

    /**
     * @brief Destroy the Consumer object
     *
     */
    ~Consumer();

    // Description of these functions can be found FileHandlerInterface class
    virtual int Open(std::string path) override;
    virtual int Close(std::string path) override;
    virtual int Fstat(struct stat *buff, std::string path) override;
    virtual ssize_t Read(void *buff, off_t offset, size_t blen,
                         std::string path) override;

  private:
    /**
     * @brief Process Interests
     *
     * @return true No exception occurred while processing Interest packets
     * @return false Exception occurred while processing the Interest packets
     */
    bool processEvents();

    /**
     * @brief Create Interest packet
     *
     * @param sc System call for which an Interest packet has to be composed
     * @param path Path to file
     * @param segmentNo Segment number of Interest packet
     * @param lifetime Lifetime of Interest packet
     * @return const ndn::Interest The resulting Interest packet
     */
    const ndn::Interest
    getInterest(ndn::Name prefix, std::string path, uint64_t segmentNo = 0,
                ndn::time::milliseconds lifetime = DEFAULT_INTEREST_LIFETIME);

    /**
     * @brief Callback function on receiving Data for open Interest
     *
     * @param interest The Interest
     * @param data The Data for the Interest
     */
    void onOpenData(const ndn::Interest &interest, const ndn::Data &data);

    /**
     * @brief Callback function on failure while processing Interest for open
     * file system call
     *
     * @param interest The Interest packet on which failure occured
     * @param reason The reason for which the failure occured
     */
    void onOpenFailure(const ndn::Interest &interest,
                       const std::string &reason);

    /**
     * @brief Callback function on receiving Data for close Interest
     *
     * @param interest The Interest
     * @param data The Data for the Interest
     */
    void onCloseData(const ndn::Interest &interest, const ndn::Data &data);

    /**
     * @brief Callback function on failure while processing Interest for close
     * file system call
     *
     * @param interest The Interest packet on which failure occured
     * @param reason The reason for which the failure occured
     */
    void onCloseFailure(const ndn::Interest &interest,
                        const std::string &reason);

    /**
     * @brief Callback function on receiving Data for fstat Interest
     *
     * @param interest The Interest
     * @param data The Data for the Interest
     */
    void onFstatData(const ndn::Interest &interest, const ndn::Data &data);

    /**
     * @brief Callback function on failure while processing Interest for fstat
     * file system call
     *
     * @param interest The Interest packet on which failure occured
     * @param reason The reason for which the failure occured
     */
    void onFstatFailure(const ndn::Interest &interest,
                        const std::string &reason);

    /**
     * @brief Callback function on receiving Data for read Interest
     *
     * @param interest The Interest
     * @param data The Data for the Interest
     */
    void onReadData(const ndn::Interest &interest, const ndn::Data &data);

    /**
     * @brief Callback function on failure while processing Interest for read
     * file system call
     *
     * @param interest The Interest packet on which failure occured
     * @param reason The reason for which the failure occured
     */
    void onReadFailure(const ndn::Interest &interest,
                       const std::string &reason);

    /**
     * @brief Put data in the provided buffer by the owner of this Consumer
     * instance
     *
     * @param buff The buffer
     * @param offset The offset in content where blen bytes were requested
     * @param blen The buffer size
     * @return off_t The actual number of bytes that have been put in the buffer
     */
    size_t returnData(void *buff, off_t offset, size_t blen);

  private:
    /**
     * @brief Get the Integer From Data object
     *
     * @param data The Data
     * @return int The Integer stored in the Data
     */
    static int getIntegerFromData(const ndn::Data &data) {
        int value = readNonNegativeInteger(data.getContent());
        return data.getContentType() == xrdndn::tlv::negativeInteger ? -value
                                                                     : value;
    }

  private:
    ndn::Face m_face;
    ndn::security::v2::Validator &m_validator;

    bool m_nfdConnected;

    struct FileStat m_FileStat;
    std::map<uint64_t, std::shared_ptr<const ndn::Data>> m_dataStore;
    std::shared_ptr<Pipeline> m_pipeline;
};
} // namespace xrdndnconsumer

#endif // XRDNDN_CONSUMER_HH