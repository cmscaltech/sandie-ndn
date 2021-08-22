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
#include <iostream>
#include <unistd.h>

#include "client.hpp"
#include "json_helper.hpp"

using json = nlohmann::json;

namespace ndnc {
namespace mgmt {
Client::Client() : m_faceID{}, m_gqlserver{} {
    auto pid = std::to_string(getpid());
    auto timestamp = std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count());

    m_socketName = "/run/ndn/ndnc-memif-" + pid + "-" + timestamp + ".sock";
}

Client::~Client() {}

bool Client::openFace(int id, int dataroom, std::string gqlserver) {
    this->m_gqlserver = gqlserver;

    auto request = json_helper::getOperation(
        "\
    mutation createFace($locator: JSON!) {\n\
      createFace(locator: $locator) {\n\
        id\n\
      }\n\
    }",
        "createFace",
        nlohmann::json{
            {"locator", json_helper::createFace{m_socketName, id, dataroom}}});

    json response;
    if (auto code = doOperation(request, response, m_gqlserver);
        CURLE_OK != code) {
        std::cout << "ERROR: createFace POST: " << curl_easy_strerror(code)
                  << ". Double check gqlserver app argument value\n";
        return false;
    }

    if (response["data"] == nullptr) {
        if (response["errors"] != nullptr) {
            for (auto error : response["errors"]) {
                std::cout << "ERROR: " << error["path"] << ": "
                          << error["message"] << "\n";
            }
        }

        return false;
    }

    if (response["data"]["createFace"] == nullptr ||
        response["data"]["createFace"]["id"] == nullptr) {
        return false;
    }

    this->m_faceID = response["data"]["createFace"]["id"];
    std::cout << "INFO: Face " << m_faceID << " opened\n";

    return true;
}

bool Client::deleteFace() {
    std::cout << "TRACE: Deleting face\n";

    auto request = json_helper::getOperation(
        "\
    mutation delete($id: ID!) {\n\
      delete(id: $id)\n\
    }",
        "delete", json_helper::deleteFace{this->m_faceID});

    json response;
    if (auto code = doOperation(request, response, m_gqlserver);
        CURLE_OK != code) {
        std::cout << "ERROR: delete POST: " << curl_easy_strerror(code) << "\n";
        return false;
    }

    if (response["data"] == nullptr || response["data"]["delete"] == nullptr) {
        return false;
    }

    return response["data"]["delete"];
}

bool Client::advertiseOnFace(const std::string prefix) {
    std::cout << "INFO: Advertising: " << prefix << "\n";

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
        std::cout << "ERROR: insertFibEntry POST: " << curl_easy_strerror(code)
                  << "\n";
        return false;
    }

    if (response["data"] == nullptr) {
        if (response["errors"] != nullptr) {
            for (auto error : response["errors"]) {
                std::cout << "ERROR: " << error["path"] << ": "
                          << error["message"] << "\n";
            }
        }

        return false;
    }

    if (response["data"]["insertFibEntry"] == nullptr ||
        response["data"]["insertFibEntry"]["id"] == nullptr ||
        response["data"]["insertFibEntry"]["id"].empty()) {
        std::cout << "ERROR: Unable to advertise Name\n";
        return false;
    }

    this->m_fibEntryID = response["data"]["insertFibEntry"]["id"];
    std::cout << "TRACE: FIB entry: " << this->m_fibEntryID
              << " for: " << prefix << "\n";

    return true;
}

}; // namespace mgmt
}; // namespace ndnc
