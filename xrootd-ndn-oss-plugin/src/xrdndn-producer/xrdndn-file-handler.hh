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

#include <boost/date_time/posix_time/posix_time.hpp>

#include "xrdndn-packager.hh"

namespace xrdndnproducer {

class FileHandler : public std::enable_shared_from_this<FileHandler> {

    using onDataCallback = std::function<void(const ndn::Data &data)>;

  public:
    static std::shared_ptr<FileHandler>
    getFileHandler(const std::string path,
                   const std::shared_ptr<Packager> &packager);

    FileHandler(const std::string path,
                const std::shared_ptr<Packager> &packager);
    ~FileHandler();

    const ndn::Data getOpenData(ndn::Name &name);
    const ndn::Data getFstatData(ndn::Name &name);
    const ndn::Data getReadData(ndn::Name &name);

    bool isOpened();
    boost::posix_time::ptime getAccessTime();

  private:
    int Open();
    int Fstat(void *buff);
    ssize_t Read(void *buff, size_t count, off_t offset);

  private:
    boost::posix_time::ptime accessTime;

    int m_fd;
    const std::string m_path;
    const std::shared_ptr<Packager> m_packager;
};
} // namespace xrdndnproducer

#endif // XRDNDN_FILE_HANDLER