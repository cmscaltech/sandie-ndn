/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2022 California Institute of Technology
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

#ifndef NDNC_UTILS_MEASUREMENTS_REPORTER_HPP
#define NDNC_UTILS_MEASUREMENTS_REPORTER_HPP

#include <limits.h>
#include <mutex>
#include <unistd.h>

#include <InfluxDBFactory.h>

#include "logger/logger.hpp"

namespace ndnc {
class MeasurementsReporter {
  public:
    MeasurementsReporter(std::size_t batch_size = 128) {
        this->influxdb_ = nullptr;
        this->batch_size_ = batch_size;
        this->hostname_ = "";
        this->id_ = "";
    }

    ~MeasurementsReporter() {
        if (influxdb_ != nullptr) {
            influxdb_->flushBatch();
            influxdb_.release();
        }
    }

    bool init(std::string id, std::string url) {
        try {
            influxdb_ = influxdb::InfluxDBFactory::Get(url);
        } catch (...) {
            LOG_WARN("unable to get influx db factory. data will not be "
                     "submitted...");
            return false;
        }

        if (influxdb_ == nullptr) {
            LOG_WARN(
                "null influxdb factory ptr. data will not be submitted...");
            return false;
        }

        influxdb_->batchOf(batch_size_);

        char hostname[HOST_NAME_MAX];
        if (gethostname(hostname, HOST_NAME_MAX) == 0) {
            hostname_ = std::string(hostname);
        } else {
            LOG_WARN("unable to get hostname");
        }

        this->id_ = hostname_ + "_" + id + "_" + std::to_string(getpid());

        return true;
    }

    void write(int64_t tx, int64_t rx, int64_t bytes, int64_t msec) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (this->influxdb_ == nullptr) {
            return;
        }

        this->influxdb_->write(influxdb::Point{"ndnc_consumer"}
                                   .addField("tx", tx)
                                   .addField("rx", rx)
                                   .addField("bytes", bytes)
                                   .addField("mean_delay_msec", msec)
                                   .addTag("id", id_)
                                   .addTag("hostname", hostname_));
    }

  private:
    std::mutex mutex_;
    std::unique_ptr<influxdb::InfluxDB> influxdb_;
    std::size_t batch_size_;
    std::string hostname_;
    std::string id_;
};
}; // namespace ndnc

#endif // NDNC_UTILS_MEASUREMENTS_REPORTER_HPP
