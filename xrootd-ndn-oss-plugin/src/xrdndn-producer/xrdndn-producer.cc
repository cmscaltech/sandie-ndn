// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

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
    : m_face(face), m_error(false) {
    NDN_LOG_TRACE("Alloc XRootD NDN Producer");

    try {
        m_face.processEvents();
    } catch (const std::exception &e) {
        m_error = true;
        NDN_LOG_ERROR(e.what());
        return;
    }

    m_interestManager = std::make_shared<InterestManager>(
        opts, std::bind(&Producer::onData, this, _1));

    if (!m_interestManager) {
        NDN_LOG_FATAL("Unable to get Interest Manager object instance");
        m_error = true;
        return;
    }

    this->registerPrefix();
}

Producer::~Producer() {
    m_xrdndnPrefixHandle.cancel();
    m_openFilterHandle.cancel();
    m_fstatFilterHandle.cancel();
    m_readFilterHandle.cancel();
    m_face.shutdown();
}

void Producer::registerPrefix() {
    m_xrdndnPrefixHandle = m_face.registerPrefix(
        xrdndn::SYS_CALLS_PREFIX_URI,
        [](const Name &name) {
            NDN_LOG_INFO("Successfully registered Interest prefix: "
                         << name << " with connected NDN forwarder");
        },
        [this](const Name &name, const std::string &msg) {
            m_error = true;
            NDN_LOG_FATAL("Could not register Interest prefix: "
                          << name << " with connected NDN forwarder: " << msg);
        });

    if (m_error)
        return;

    m_openFilterHandle =
        m_face.setInterestFilter(xrdndn::SYS_CALL_OPEN_PREFIX_URI,
                                 bind(&Producer::onOpenInterest, this, _1, _2));
    NDN_LOG_INFO("Set Interest filter: " << xrdndn::SYS_CALL_OPEN_PREFIX_URI);

    m_fstatFilterHandle = m_face.setInterestFilter(
        xrdndn::SYS_CALL_FSTAT_PREFIX_URI,
        bind(&Producer::onFstatInterest, this, _1, _2));
    NDN_LOG_INFO("Set Interest filter: " << xrdndn::SYS_CALL_FSTAT_PREFIX_URI);

    m_readFilterHandle =
        m_face.setInterestFilter(xrdndn::SYS_CALL_READ_PREFIX_URI,
                                 bind(&Producer::onReadInterest, this, _1, _2));
    NDN_LOG_INFO("Set Interest filter: " << xrdndn::SYS_CALL_READ_PREFIX_URI);
}

void Producer::onData(std::shared_ptr<ndn::Data> data) {
    NDN_LOG_TRACE("Sending Data: " << data);
    m_face.put(*data);
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