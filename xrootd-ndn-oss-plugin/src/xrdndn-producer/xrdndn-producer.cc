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

#include "xrdndn-producer.hh"
#include "../common/xrdndn-logger.hh"
#include "../common/xrdndn-namespace.hh"

using namespace ndn;

namespace xrdndnproducer {
std::shared_ptr<Producer>
Producer::getXrdNdnProducerInstance(Face &face, const Options &opts) {
    auto producer = std::make_shared<Producer>(face, opts);
    if (!producer || producer->m_error) {
        NDN_LOG_FATAL("Unable to get XRootD NDN Producer object instance");
        return nullptr;
    }

    return producer;
}

Producer::Producer(Face &face, const Options &opts)
    : m_face(face), m_error(false), m_OpenFilterId(nullptr),
      m_ReadFilterId(nullptr) {

    NDN_LOG_TRACE("Alloc XRootD NDN Producer");

    try {
        m_face.processEvents();
    } catch (const std::exception &e) {
        m_error = true;
        NDN_LOG_ERROR(e.what());
    }

    m_interestManager = std::make_shared<InterestManager>(
        opts, std::bind(&Producer::onData, this, _1));
    if (!m_interestManager) {
        NDN_LOG_ERROR("Unable to get Interest Manager object instance");
        m_error = true;
    }

    if (!m_error)
        this->registerPrefix();
}

Producer::~Producer() {
    if (m_xrdndnPrefixId != nullptr) {
        m_face.unsetInterestFilter(m_xrdndnPrefixId);
    }
    if (m_OpenFilterId != nullptr) {
        m_face.unsetInterestFilter(m_OpenFilterId);
    }
    if (m_FstatFilterId != nullptr) {
        m_face.unsetInterestFilter(m_FstatFilterId);
    }
    if (m_ReadFilterId != nullptr) {
        m_face.unsetInterestFilter(m_ReadFilterId);
    }

    m_face.shutdown();
}

// Register all interest filters that this producer will answer to
void Producer::registerPrefix() {
    NDN_LOG_TRACE("Register prefixes.");

    // For nfd
    m_xrdndnPrefixId = m_face.registerPrefix(
        xrdndn::SYS_CALLS_PREFIX_URI,
        [](const Name &name) {
            NDN_LOG_INFO("Successfully registered Interest prefix: " << name);
        },
        [](const Name &name, const std::string &msg) {
            NDN_LOG_ERROR("Could not register " << name
                                                << " prefix for nfd: " << msg);
        });
    if (!m_xrdndnPrefixId)
        m_error = true;

    // Filter for open system call
    m_OpenFilterId =
        m_face.setInterestFilter(xrdndn::SYS_CALL_OPEN_PREFIX_URI,
                                 bind(&Producer::onOpenInterest, this, _1, _2));
    if (!m_OpenFilterId) {
        m_error = true;
        NDN_LOG_ERROR("Could not set Interest filter for open systemcall.");
    } else {
        NDN_LOG_INFO("Successfully set Interest filter: "
                     << xrdndn::SYS_CALL_OPEN_PREFIX_URI);
    }

    // Filter for fstat system call
    m_FstatFilterId = m_face.setInterestFilter(
        xrdndn::SYS_CALL_FSTAT_PREFIX_URI,
        bind(&Producer::onFstatInterest, this, _1, _2));
    if (!m_FstatFilterId) {
        m_error = true;
        NDN_LOG_ERROR("Could not set Interest filter for fstat systemcall.");
    } else {
        NDN_LOG_INFO("Successfully set Interest filter: "
                     << xrdndn::SYS_CALL_FSTAT_PREFIX_URI);
    }

    // Filter for read system call
    m_ReadFilterId =
        m_face.setInterestFilter(xrdndn::SYS_CALL_READ_PREFIX_URI,
                                 bind(&Producer::onReadInterest, this, _1, _2));
    if (!m_ReadFilterId) {
        m_error = true;
        NDN_LOG_ERROR("CCould not set Interest filter for read systemcall.");
    } else {
        NDN_LOG_INFO("Successfully set Interest filter: "
                     << xrdndn::SYS_CALL_READ_PREFIX_URI);
    }
}

void Producer::onData(const ndn::Data &data) {
    NDN_LOG_TRACE("Sending Data: " << data);
    m_face.put(data);
}

void Producer::onOpenInterest(const InterestFilter &,
                              const Interest &interest) {
    NDN_LOG_TRACE("onOpenInterest: " << interest);
    m_interestManager->openInterest(interest);
}

void Producer::onFstatInterest(const ndn::InterestFilter &,
                               const ndn::Interest &interest) {
    NDN_LOG_TRACE("onFstatInterest: " << interest);
    m_interestManager->fstatInterest(interest);
}

void Producer::onReadInterest(const InterestFilter &,
                              const Interest &interest) {
    NDN_LOG_TRACE("onReadInterest: " << interest);
    m_interestManager->readInterest(interest);
}
} // namespace xrdndnproducer