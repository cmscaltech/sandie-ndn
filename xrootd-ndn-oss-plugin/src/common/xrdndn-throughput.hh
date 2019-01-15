/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2019 California Institute of Technology                        *
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

#include <string>

#include "xrdndn-common.hh"
#include "xrdndn-logger.hh"
#include <ndn-cxx/util/time.hpp>

namespace xrdndnconsumer {
class ThroughputComputation {
  public:
    ThroughputComputation() {}

    ~ThroughputComputation() {}

    void start() { m_startTime = ndn::time::steady_clock::now(); }
    void stop() { m_endTime = ndn::time::steady_clock::now(); }

    void printSummary(int64_t nSegmentsReceived = 0, int64_t nBytesReceived = 0,
                      std::string fileName = "") {
        ndn::time::duration<double, ndn::time::milliseconds::period>
            timeElapsed = m_endTime - m_startTime;

        double throughput = (8 * nBytesReceived * 1000) / timeElapsed.count();

        std::cout << "Transfer for file: " << fileName << " over NDN completed."
                  << "\nTime elapsed: " << timeElapsed
                  << "\nTotal # of segments received:" << nSegmentsReceived
                  << "\nTotal size: "
                  << static_cast<double>(nBytesReceived) / 1000 << " kB"
                  << "\nThroughput: " << formatThroughput(throughput) << "\n\n";
    }

  private:
    std::string formatThroughput(double throughput) {
        int pow = 0;
        while (throughput >= 1000.0 && pow < 4) {
            throughput /= 1000.0;
            pow++;
        }
        switch (pow) {
        case 0:
            return std::to_string(throughput) + " bit/s";
        case 1:
            return std::to_string(throughput) + " kbit/s";
        case 2:
            return std::to_string(throughput) + " Mbit/s";
        case 3:
            return std::to_string(throughput) + " Gbit/s";
        case 4:
            return std::to_string(throughput) + " Tbit/s";
        }
        return "";
    }

  private:
    ndn::time::steady_clock::TimePoint m_startTime;
    ndn::time::steady_clock::TimePoint m_endTime;
};
} // namespace xrdndnconsumer
