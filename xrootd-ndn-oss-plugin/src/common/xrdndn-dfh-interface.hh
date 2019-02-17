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

#ifndef XRDNDN_DFH_INTERFACE_HH
#define XRDNDN_DFH_INTERFACE_HH

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ndn-cxx/name.hpp>

namespace xrdndn {

/**
 * @brief Name prefix for all Interest packets expressed by Consumer
 *
 */
static const ndn::Name SYS_CALLS_PREFIX_URI("/ndn/xrootd/");
/**
 * @brief Keep the number of components in each prefix
 *
 */
static const uint8_t SYS_CALLS_PREFIX_LEN = SYS_CALLS_PREFIX_URI.size() + 1;

/**
 * @brief Name filter for open system call Interest packet
 *
 */
static const ndn::Name SYS_CALL_OPEN_PREFIX_URI("/ndn/xrootd/open/");
/**
 * @brief Name filter for close system call Interest packet
 *
 */
static const ndn::Name SYS_CALL_CLOSE_PREFIX_URI("/ndn/xrootd/close/");
/**
 * @brief Name filter for fstat system call Interest packet
 *
 */
static const ndn::Name SYS_CALL_FSTAT_PREFIX_URI("/ndn/xrootd/fstat/");
/**
 * @brief Name filter for read system call Interest packet
 *
 */
static const ndn::Name SYS_CALL_READ_PREFIX_URI("/ndn/xrootd/read/");

class FileHandlerInterface {
    /**
     * @brief This is the abstract class of file handler system calls
     * implemented by Consumer and Producer
     *
     */

  public:
    /**
     * @brief Destroy the File Handler Interface object
     *
     */
    virtual ~FileHandlerInterface() {}

    /**
     * @brief Open file function over NDN. On the Consumer side, translation to
     * a corresponding Interest packet will take place. On the Producer side,
     * the file will be opened
     *
     * @param path The file name
     * @return int The return value of open POSIX system call on the Producer
     * side. 0 (success) / -errno (error)
     */
    virtual int Open(std::string path) = 0;

    /**
     * @brief Close file function over NDN. On the Consumer side, translation to
     * a corresponding Interest packet will take place. On the Producer side,
     * the file will be closed
     *
     * @param path The file name
     * @return int The return value of close POSIX system call on the Producer
     * side. 0 (success) / -errno (error)
     */
    virtual int Close(std::string path) = 0;

    /**
     * @brief Fstat file function over NDN. On the Consumer side, translation to
     * a corresponding Interest packet will take place. On the Producer side,
     * fstat will be called for file
     *
     * @param buff If fstat is possible, the POSIX struct stat of file will be
     * put in
     * @param path The file name
     * @return int The return value of fstat POSIX system call on the Producer
     * side. 0 (success and buff will have data) / -errno (error)
     */
    virtual int Fstat(struct stat *buff, std::string path) = 0;

    /**
     * @brief Read blen bytes from file over NDN. On the Consumer side, the
     * request will be translated into a number of Interest packets. On the
     * Producer side, chunks of file will be read and Data will be sent in
     * return
     *
     * @param buff The address where data will be stored
     * @param offset Offset in file were the read will begin
     * @param blen The number of bytes to be read by Producer
     * @param path The file name
     * @return ssize_t On Success the actual number of bytes read on the
     * Producer side. On failure -errno
     */
    virtual ssize_t Read(void *buff, off_t offset, size_t blen,
                         std::string path) = 0;
};
} // namespace xrdndn

#endif // XRDNDN_DFH_INTERFACE_HH