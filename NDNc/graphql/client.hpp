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

#ifndef NDNC_GRAPHQL_CLIENT_HPP
#define NDNC_GRAPHQL_CLIENT_HPP

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>

namespace ndnc {
namespace graphql {

class Client {
  public:
    Client();
    ~Client();

    bool openFace(int id = 1, int dataroom = 9000);
    bool deleteFace();
    bool advertiseOnFace(const std::string prefix);

    std::string getSocketName() { return m_socketName; }
    std::string getFaceID() { return m_faceID; }
    std::string getFibEntryID() { return m_fibEntryID; }

  private:
    static size_t writeCallback(char *ptr, size_t size, size_t nmemb,
                                void *userdata) {
        ((std::string *)userdata)->append((char *)ptr, size * nmemb);
        return size * nmemb;
    }

    static CURLcode doOperation(const nlohmann::json request,
                                nlohmann::json &response) {
        curl_global_init(CURL_GLOBAL_ALL);
        auto curl = curl_easy_init();
        if (curl == NULL) {
            return CURLE_FAILED_INIT;
        }

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        // curl_easy_setopt(curl, CURLOPT_URL, "http://172.17.0.2:3030/"); // TODO address should be configurable
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3030/");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        auto postFields = strdup(request.dump().c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);

        std::string data;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

        auto code = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        curl_global_cleanup();
        free(postFields);

        if (data.empty()) {
            return CURLE_RECV_ERROR;
        }

        response = nlohmann::json::parse(data);
        return code;
    }

  private:
    std::string m_socketName;
    std::string m_faceID;
    std::string m_fibEntryID;
};
}; // namespace graphql
}; // namespace ndnc

#endif // NDNC_GRAPHQL_CLIENT_HPP
