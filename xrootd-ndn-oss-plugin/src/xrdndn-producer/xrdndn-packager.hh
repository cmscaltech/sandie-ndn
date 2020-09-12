// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef XRDNDN_PACKAGER
#define XRDNDN_PACKAGER

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-info.hpp>

namespace xrdndnproducer {
/**
 * @brief This class implements the NDN Data Packager. Each Data response to
 * Interest produced by Producer will be composed by this object
 *
 */
class Packager : public std::enable_shared_from_this<Packager> {
    /**
     * @brief NDN SigningInfo for signing Data used by key chain
     *
     */
    static const ndn::security::SigningInfo signingInfo;
    /**
     * @brief NDN Key Chain for signing Data
     *
     */
    static const std::shared_ptr<ndn::KeyChain> keyChain;

  public:
    /**
     * @brief Construct a new Packager object
     *
     * @param freshnessPeriod Interest freshness period in milliseconds
     * @param disableSignature If true, data will not be signed with signingInfo
     * but with a dummy fake signature
     */
    Packager(uint64_t freshnessPeriod, bool disableSignature = false);

    /**
     * @brief Destroy the Packager object
     *
     */
    ~Packager();

    /**
     * @brief Get the Package object
     *
     * @param name NDN Name of Data packet
     * @param contentValue Data content as an integer
     * @return std::shared_ptr<ndn::Data> The resulted Data packet as shared_ptr
     */
    std::shared_ptr<ndn::Data> getPackage(ndn::Name &name,
                                          const int contentValue);

    /**
     * @brief Get the Package object
     *
     * @param name NDN Nmae of Data packet
     * @param value Data content as a byte buffer
     * @param size The size of the byte buffer
     * @return std::shared_ptr<ndn::Data> The resulted Data packet as shared_ptr
     */
    std::shared_ptr<ndn::Data> getPackage(ndn::Name &name, const uint8_t *value,
                                          ssize_t size);

  private:
    /**
     * @brief After the content for Data is set, this function will set the
     * final parameters of the packet and sign it
     *
     * @param data The Data for which parameteres will be set and to be singed
     */
    void digest(std::shared_ptr<ndn::Data> data);

  private:
    const ndn::time::milliseconds m_freshnessPeriod;
    bool m_disableSigning;
    ndn::Signature m_fakeSignature;
};
} // namespace xrdndnproducer

#endif // XRDNDN_PACKAGER