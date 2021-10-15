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

#ifndef NDNC_MGMT_JSON_HELPER_HPP
#define NDNC_MGMT_JSON_HELPER_HPP

#include <nlohmann/json.hpp>
#include <set>

namespace ndnc {
namespace mgmt {
/**
 * @brief GraphQL documents for NDN-DPDK forwarder configuration
 *
 */
namespace json_helper {
/**
 * @brief GraphQL server operation
 *
 */
struct operation {
    // GraphQL document
    std::string query;
    // Operation Name
    std::string operationName;
    // Query variables
    nlohmann::json variables;
};

void to_json(nlohmann::json &json, const operation &req) {
    json = nlohmann::json{{"query", req.query},
                          {"operationName", req.operationName},
                          {"variables", req.variables}};
}

void from_json(const nlohmann::json &json, operation &req) {
    json.at("query").get_to(req.query);
    json.at("operationName").get_to(req.operationName);
    json.at("variables").get_to(req.variables);
}

nlohmann::json getOperation(std::string query, std::string name,
                            nlohmann::json variables) {
    return operation{query, name, variables};
}

/**
 * @brief Query variables: createFace mutation
 *
 */
struct createFace {
    std::string socketName; // Socket name
    std::string scheme = "memif";
    int id;       // Face ID
    int dataroom; // Dataroom size
    int rxQueueSize = 1024;
    int txQueueSize = 1024;
    int ringCapacity = 4096;
};

void to_json(nlohmann::json &json, const createFace &loc) {
    json = nlohmann::json{
        {"dataroom", loc.dataroom},        {"id", loc.id},
        {"socketName", loc.socketName},    {"scheme", loc.scheme},
        {"rxQueueSize", loc.rxQueueSize},  {"txQueueSize", loc.txQueueSize},
        {"ringCapacity", loc.ringCapacity}};
}

void from_json(const nlohmann::json &json, createFace &loc) {
    json.at("dataroom").get_to(loc.dataroom);
    json.at("id").get_to(loc.id);
    json.at("socketName").get_to(loc.socketName);
    json.at("scheme").get_to(loc.scheme);
    json.at("rxQueueSize").get_to(loc.rxQueueSize);
    json.at("txQueueSize").get_to(loc.txQueueSize);
    json.at("ringCapacity").get_to(loc.ringCapacity);
}

/**
 * @brief Query variable: delete mutation
 *
 */
struct deleteFace {
    std::string id; // Face ID on the forwarder side
};

void to_json(nlohmann::json &json, const deleteFace &vars) {
    json = nlohmann::json{{"id", vars.id}};
}

void from_json(const nlohmann::json &json, deleteFace &vars) {
    json.at("id").get_to(vars.id);
}

/**
 * @brief Query variables: insertFibEntry mutation
 *
 */
struct insertFibEntry {
    std::string name;               // Name prefix
    std::set<std::string> nextHops; // Next hops: Face ID of this app
    // int strategy;
};

void to_json(nlohmann::json &json, const insertFibEntry &vars) {
    json = nlohmann::json{{"name", vars.name}, {"nexthops", vars.nextHops}};
}

void from_json(const nlohmann::json &json, insertFibEntry &vars) {
    json.at("name").get_to(vars.name);
    json.at("nexthops").get_to(vars.nextHops);
}

}; // namespace json_helper
}; // namespace mgmt
}; // namespace ndnc

#endif // NDNC_MGMT_JSON_HELPER_HPP
