/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018-2019 California Institute of Technology                   *
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

#ifndef XRDNDN_PRODUCER_HH
#define XRDNDN_PRODUCER_HH

#include <ndn-cxx/face.hpp>

#include <boost/noncopyable.hpp>

#include "xrdndn-interest-manager.hh"
#include "xrdndn-producer-options.hh"

namespace xrdndnproducer {
/**
 * @brief This class implements the Producer used be the XRootD NDN based file
 * system
 *
 */
class Producer : boost::noncopyable {
  public:
    /**
     * @brief Get the XRootF NDN Producer Instance object
     *
     * @param face The Face were Interest are received and Data sent back
     * @param opts Command line arguments
     * @return std::shared_ptr<Producer> The resulted Producer instance object
     * as shared_ptr
     */
    static std::shared_ptr<Producer>
    getXrdNdnProducerInstance(ndn::Face &face, const Options &opts);

    /**
     * @brief Construct a new Producer object
     *
     * @param face The Face were Interest are received and Data sent back
     * @param opts Command line arguments
     */
    Producer(ndn::Face &face, const Options &opts);

    /**
     * @brief Destroy the Producer object
     *
     */
    ~Producer();

  private:
    /**
     * @brief Register prefix to local NDN forwarder and set Interest filters
     *
     */
    void registerPrefix();

    /**
     * @brief Function called be InterestManager object to put Data on Face
     *
     * @param data The Data to be sent back to the Consumer
     */
    void onData(std::shared_ptr<ndn::Data> data);

    /**
     * @brief Function called when Interest for open system call is received on
     * Face
     *
     * @param interest The Interest
     */
    void onOpenInterest(const ndn::InterestFilter &,
                        const ndn::Interest &interest);

    /**
     * @brief Function called when Interest for fstat system call is received on
     * Face
     *
     * @param interest The Interest
     */
    void onFstatInterest(const ndn::InterestFilter &,
                         const ndn::Interest &interest);

    /**
     * @brief Function called when Interest for read system call is received on
     * Face
     *
     * @param interest The Interest
     */
    void onReadInterest(const ndn::InterestFilter &,
                        const ndn::Interest &interest);

  private:
    ndn::Face &m_face;
    bool m_error;

    ndn::RegisteredPrefixHandle m_xrdndnPrefixHandle;
    ndn::InterestFilterHandle m_openFilterHandle;
    ndn::InterestFilterHandle m_fstatFilterHandle;
    ndn::InterestFilterHandle m_readFilterHandle;

    std::shared_ptr<InterestManager> m_interestManager;
};

} // namespace xrdndnproducer

#endif // XRDNDN_PRODUCER_HH