/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2021 California Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <chrono>
#include <unistd.h>

#include "client.hpp"
#include "json_helper.hpp"
#include "logger/logger.hpp"

using json = nlohmann::json;

namespace ndnc {
namespace mgmt {
Client::Client() : m_gqlserver{}, m_faceID{}, m_fibEntryID{} {
    auto pid = std::to_string(getpid());
    auto timestamp = std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count());

    m_socketPath = "/run/ndn/ndnc-memif-" + pid + "-" + timestamp + ".sock";
}

Client::~Client() {}

bool Client::createFace(int id, int dataroom, std::string gqlserver) {
    this->m_gqlserver = gqlserver;

    auto request = json_helper::getOperation(
        "\
    mutation createFace($locator: JSON!) {\n\
      createFace(locator: $locator) {\n\
        id\n\
      }\n\
    }",
        "createFace",
        nlohmann::json{{"locator", json_helper::createFace{
                                       m_socketPath, "memif", id, dataroom}}});

    json response;
    if (auto code = doOperation(request, response, m_gqlserver);
        CURLE_OK != code) {
        LOG_ERROR("createFace mutation POST result=%s. Hint: double check "
                  "GraphQL Server address",
                  curl_easy_strerror(code));
        return false;
    }

    if (response["data"] == nullptr) {
        if (response["errors"] != nullptr) {
            for (auto error : response["errors"]) {
                LOG_ERROR("%s: %s", std::string(error["path"]).c_str(),
                          std::string(error["message"]).c_str());
            }
        }
        return false;
    }

    if (response["data"]["createFace"] == nullptr ||
        response["data"]["createFace"]["id"] == nullptr) {
        LOG_ERROR("createFace mutation response data has null values");
        return false;
    }

    this->m_faceID = response["data"]["createFace"]["id"];
    LOG_INFO("createFace mutation done. id=%s", m_faceID.c_str());
    return true;
}

bool Client::insertFibEntry(const std::string prefix) {
    auto request = json_helper::getOperation(
        "\
    mutation insertFibEntry($name: Name!, $nexthops: [ID!]!, $strategy: ID) {\n\
        insertFibEntry(name: $name, nexthops: $nexthops, strategy: $strategy) {\n\
            id\n\
        }\n\
    }",
        "insertFibEntry",
        json_helper::insertFibEntry{prefix, {this->m_faceID}});

    json response;
    if (auto code = doOperation(request, response, m_gqlserver);
        CURLE_OK != code) {
        LOG_ERROR("insertFibEntry mutation POST result=%s",
                  curl_easy_strerror(code));
        return false;
    }

    if (response["data"] == nullptr) {
        if (response["errors"] != nullptr) {
            for (auto error : response["errors"]) {
                LOG_ERROR("%s: %s", std::string(error["path"]).c_str(),
                          std::string(error["message"]).c_str());
            }
        }
        return false;
    }

    if (response["data"]["insertFibEntry"] == nullptr ||
        response["data"]["insertFibEntry"]["id"] == nullptr ||
        response["data"]["insertFibEntry"]["id"].empty()) {
        LOG_ERROR("unable to insert FIB entry");
        return false;
    }

    this->m_fibEntryID = response["data"]["insertFibEntry"]["id"];
    LOG_INFO("insertFibEntry mutation done. id=%s for prefix=%s",
             m_fibEntryID.c_str(), prefix.c_str());
    return true;
}

bool Client::deleteFace() {
    if (!this->m_fibEntryID.empty()) {
        LOG_DEBUG("delete FIB entry id=%s", m_fibEntryID.c_str());
        this->deleteID(this->m_fibEntryID);
    }

    LOG_DEBUG("delete face id=%s", m_faceID.c_str());
    return deleteID(this->m_faceID);
}

bool Client::deleteID(std::string id) {
    auto request =
        json_helper::getOperation("\
    mutation delete($id: ID!) {\n\
      delete(id: $id)\n\
    }",
                                  "delete", json_helper::deleteFace{id});

    json response;
    if (auto code = doOperation(request, response, m_gqlserver);
        CURLE_OK != code) {
        LOG_ERROR("delete mutation POST result=%s", curl_easy_strerror(code));
        return false;
    }

    if (response["data"] == nullptr || response["data"]["delete"] == nullptr) {
        LOG_ERROR("delete mutation response data has null values");
        return false;
    }

    LOG_INFO("delete mutation done. id=%s", id.c_str());
    return response["data"]["delete"];
}
}; // namespace mgmt
}; // namespace ndnc
