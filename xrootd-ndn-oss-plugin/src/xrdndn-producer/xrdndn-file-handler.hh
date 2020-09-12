// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
//
// Author: Catalin Iordache <catalin.iordache@cern.ch>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef XRDNDN_FILE_HANDLER
#define XRDNDN_FILE_HANDLER

#include <boost/date_time/posix_time/posix_time.hpp>

#include "xrdndn-packager.hh"

namespace xrdndnproducer {
/**
 * @brief This class implements the File Handler object. All file system calls
 * on a specific file are handled by this object
 *
 */
class FileHandler : public std::enable_shared_from_this<FileHandler> {
  public:
    /**
     * @brief Get the File Handler object
     *
     * @param path Path to the file
     * @param packager Packager instance object
     * @return std::shared_ptr<FileHandler> A FileHandler object as shared_ptr
     */
    static std::shared_ptr<FileHandler>
    getFileHandler(const std::string path,
                   const std::shared_ptr<Packager> &packager);

    /**
     * @brief Construct a new File Handler object
     *
     * @param path Path to the file
     * @param packager Packager instance object
     */
    FileHandler(const std::string path,
                const std::shared_ptr<Packager> &packager);

    /**
     * @brief Destroy the File Handler object
     *
     */
    ~FileHandler();

    /**
     * @brief Opens the file and returns Data with integer content regarding the
     * result of opening the file
     *
     * @param name The Name of Data packet
     * @return std::shared_ptr<ndn::Data> The resulted Data packet
     */
    std::shared_ptr<ndn::Data> getOpenData(ndn::Name &name);

    /**
     * @brief Fstat on the file.
     *
     * @param name The Name of Data packet
     * @return std::shared_ptr<ndn::Data> The resulted Data packet. Integer
     * content in case fstat failed on the specific file, showing the error
     * code. Struct stat content on success, returning all information regarding
     * the file
     */
    std::shared_ptr<ndn::Data> getFstatData(ndn::Name &name);

    /**
     * @brief Read at offset from file XRDNDN_MAX_PAYLOAD_SIZE bytes
     *
     * @param name The Name of Data packet. From the name the offset in file
     * were the read will take place is also extracted
     * @return std::shared_ptr<ndn::Data> The resulted Data packet. Integer
     * content in case read failed on the specific file, showing the error
     * code. The XRDNDN_MAX_PAYLOAD_SIZE bytes or less, if the read was near
     * end of file
     */
    std::shared_ptr<ndn::Data> getReadData(ndn::Name &name);

    /**
     * @brief Check if file is open
     *
     * @return true File is open
     * @return false File is not open
     */
    bool isOpened();

    /**
     * @brief Get the last time this object was accessed. Used by garbage
     * collector
     *
     * @return boost::posix_time::ptime The last time this object was accessed
     */
    boost::posix_time::ptime getAccessTime();

  private:
    /**
     * @brief Opens the file using POSIX system call with O_RDONLY permition, as
     * it is enough for NDN
     *
     * @return int 0 on success. -errno on failure
     */
    int Open();

    /**
     * @brief Fstat the file using POSIX system call.
     *
     * @param buff The buffer were the resulted struct stat will be copied
     * @return int 0 on success. -errno on failure
     */
    int Fstat(void *buff);

    /**
     * @brief Read count bytes at offset from file and store the data in buff
     *
     * @param buff Were the bytes from file will be stored
     * @param count Maximum number of bytes to be read
     * @param offset Offset in file
     * @return ssize_t On success, the actual number of bytes that were read
     * from the file. -errno on failure
     */
    ssize_t Read(void *buff, size_t count, off_t offset);

  private:
    boost::posix_time::ptime accessTime;
    int m_fd;
    const std::string m_path;
    const std::shared_ptr<Packager> m_packager;
};
} // namespace xrdndnproducer

#endif // XRDNDN_FILE_HANDLER