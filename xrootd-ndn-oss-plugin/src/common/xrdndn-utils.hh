// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef XRDNDN_UTILS_HH
#define XRDNDN_UTILS_HH

#include <iostream>

#include <ndn-cxx/name.hpp>

#include "xrdndn-namespace.hh"

namespace xrdndn {
class Utils {
  public:
    /**
     * @brief Get the Name object for a prefix, a file name and a segment number
     *
     * @param prefix The prefix of the Name
     * @param path The file path
     * @param segmentNo The segment number
     * @return ndn::Name The resulting NDN Name
     */
    static ndn::Name getName(ndn::Name prefix, std::string path,
                             uint64_t segmentNo = 0) noexcept {
        return prefix == SYS_CALL_READ_PREFIX_URI
                   ? prefix.append(path).appendSegment(segmentNo)
                   : prefix.append(path);
    }

    /**
     * @brief Get the file name from an Interest Name
     *
     * @param name The Name
     * @return std::string The file name as a std::string
     */
    static std::string getPath(const ndn::Name &name) noexcept {
        try {
            if (name.at(-1).isSegment())
                return name.getSubName(xrdndn::SYS_CALLS_PREFIX_LEN)
                    .getPrefix(-1)
                    .toUri();
        } catch (ndn::Name::Error &error) {
            std::cerr << "Unable to get file path from Name: " << name
                      << std::endl;
            return std::string();
        }
        return name.getSubName(xrdndn::SYS_CALLS_PREFIX_LEN).toUri();
    }

    /**
     * @brief Get segment number from Name
     *
     * @param name Name
     * @return uint64_t Segment number
     */
    static uint64_t getSegmentNo(const ndn::Name &name) noexcept {
        try {
            if (name.at(-1).isSegment())
                return name.at(-1).toSegment();
            else if (name.at(-1).isVersion())
                return name.at(-2).toSegment();
        } catch (ndn::Name::Error &error) {
            std::cerr << "Unable to get segment number from name: " << name
                      << std::endl;
        }

        return 0;
    }
};
} // namespace xrdndn

#endif // XRDNDN_UTILS_HH