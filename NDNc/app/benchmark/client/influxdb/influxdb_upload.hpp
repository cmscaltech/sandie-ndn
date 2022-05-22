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

#ifndef NDNC_APP_BENCHMARK_INFLUXDB_UPLOAD_HPP
#define NDNC_APP_BENCHMARK_INFLUXDB_UPLOAD_HPP

#include <curl/multi.h>
#include <string>
#include <unistd.h>

#include "logger/logger.hpp"

namespace ndnc {
struct InfluxDBDataPoint {
    std::string file = "";
    uint64_t nBytes = 0;
    uint64_t nData = 0;
    double goodput = 0.0;

    std::string measurement() {
        return "progress";
    }

    std::string tagSet() {
        std::string hostnameTag = "hostname=";
        std::string pidTag = "pid=" + std::to_string(getpid()) + "i";

        char hostname[128];
        gethostname(hostname, 128);
        hostnameTag += "\"" + std::string(hostname) + "\"";

        return hostnameTag + "," + pidTag;
    }

    std::string fieldSet() {
        std::string value = "";

        value += "file=\"" + file + "\"";
        value += ",byte_count=" + std::to_string(nBytes) + "i";
        value += ",goodput=" + std::to_string(goodput);
        value += ",data_count=" + std::to_string(nData) + "i";

        return value;
    }

    std::string get() {
        return measurement() + "," + tagSet() + " " + fieldSet();
    }
};
class InfluxDBClient {
  public:
    InfluxDBClient(std::string dbURL, std::string db, std::string username = "",
                   std::string password = "") {
        this->m_dbURL = dbURL;
        this->m_db = db;
        this->m_username = username;
        this->m_password = password;

        if (curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK) {
            m_curl = curl_easy_init();
        }

        m_multi_handle = curl_multi_init();
    }

    ~InfluxDBClient() {
        curl_multi_cleanup(m_multi_handle);

        if (m_curl != nullptr) {
            curl_easy_cleanup(m_curl);
        }
        curl_global_cleanup();
    }

    bool isValid() {
        return m_curl != nullptr && !m_dbURL.empty() && !m_db.empty();
    }

    CURLcode uploadData(InfluxDBDataPoint dataPoint) {
        if (!isValid()) {
            return CURLE_FAILED_INIT;
        }

        struct curl_slist *headers = nullptr;
        curl_slist_append(headers,
                          "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(m_curl, CURLOPT_POST, 1L);

        // "http://localhost:8086/write?db=sc21&u=admin&p=admin"
        auto curlPOSTURL = m_dbURL + "/write?db=" + m_db + "&u=" + m_username +
                           "&p=" + m_password;
        curl_easy_setopt(m_curl, CURLOPT_URL, curlPOSTURL.c_str());
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

        char body[8196];
        sprintf(body, "%s   \n", dataPoint.get().c_str());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, body);

        LOG_DEBUG("%s", dataPoint.get().c_str());

        int still_running;

        curl_multi_add_handle(m_multi_handle, m_curl);
        curl_multi_perform(m_multi_handle, &still_running);

        // CURLcode code = curl_easy_perform(m_curl);
        // if (code != CURLE_OK) {
        //     LOG_ERROR("curl_easy_perform() failed: %s",
        //               curl_easy_strerror(code));
        // }

        curl_slist_free_all(headers);
        return CURLE_OK;
    }

  private:
    std::string m_dbURL;
    std::string m_db;
    std::string m_username;
    std::string m_password;

    CURL *m_curl;
    CURLM *m_multi_handle;
};
}; // namespace ndnc

#endif // NDNC_APP_BENCHMARK_INFLUXDB_UPLOAD_HPP
