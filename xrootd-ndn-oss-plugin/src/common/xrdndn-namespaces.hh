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

#ifndef XRDNDN_NAMESPACES_HH
#define XRDNDN_NAMESPACES_HH

#include <boost/preprocessor.hpp>

namespace xrdndn {
#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem)     \
    case elem:                                                                 \
        return BOOST_PP_STRINGIZE(elem);

#define DEFINE_ENUM_WITH_STRING_CONVERSIONS(name, enumerators)                 \
    enum name { BOOST_PP_SEQ_ENUM(enumerators) };                              \
                                                                               \
    inline const char *enumToString(name v) {                                  \
        switch (v) {                                                           \
            BOOST_PP_SEQ_FOR_EACH(                                             \
                X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE, name,     \
                enumerators)                                                   \
        default:                                                               \
            return "[Unknown " BOOST_PP_STRINGIZE(name) "]";                   \
        }                                                                      \
    }

#define PLUGIN_INTEREST_PREFIX_URI "/ndn/xrootd/"
} // namespace xrdndn

#endif // XRDNDN_NAMESPACES_HH