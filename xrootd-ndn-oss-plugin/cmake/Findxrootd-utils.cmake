function(print_message mode VALUE)
  if(NOT xrootd-utils_FIND_QUIETLY)
    message(${mode} ${VALUE})
  endif()
endfunction()

print_message(STATUS "Looking for XrdVersion.hh")
find_path(XROOTD_INCLUDES XrdVersion.hh
          HINTS "/usr/" "/opt/xrootd"
          PATH_SUFFIXES "include/xrootd"
          PATHS "/opt/xrootd")

if(XROOTD_INCLUDES-NOTFOUND)
  print_message(STATUS "Looking for XrdVersion.hh - not found")
  set(XROOTD-NOTFOUND TRUE)
else()
  # Set XRootD version
  file(READ "${XROOTD_INCLUDES}/XrdVersion.hh" xrootd-version-content)
  string(REGEX MATCH
               "XrdVERSION(.*)([0-9]+)\\.([0-9]+)\\.([0-9]+)[\"]"
               XROOTD-VERSION
               ${xrootd-version-content})

  string(REPLACE "XrdVERSION"
                 ""
                 XROOTD-VERSION
                 ${XROOTD-VERSION})
  string(REPLACE " "
                 ""
                 XROOTD-VERSION
                 ${XROOTD-VERSION})
  string(REPLACE "\""
                 ""
                 XROOTD-VERSION
                 ${XROOTD-VERSION})
  string(REPLACE "v"
                 ""
                 XROOTD-VERSION
                 ${XROOTD-VERSION})

  print_message(STATUS "Looking for XrdVersion.hh - found: ${XROOTD_INCLUDES}")
  set(XROOTD-FOUND TRUE)
endif(XROOTD_INCLUDES-NOTFOUND)

print_message(STATUS "Looking for libXrdUtils")
find_library(XROOTD_UTILS XrdUtils
             HINTS "/usr/" "/opt/xrootd"
             PATHS "/opt/xrootd")

if(XROOTD_UTILS-NOTFOUND)
  print_message(STATUS "Looking for libnXrdUtils- not found")
  set(XROOTD-NOTFOUND TRUE)
else()
  print_message(STATUS "Looking for libXrdUtils - found: ${XROOTD_UTILS}")
  set(XROOTD-FOUND TRUE)
endif(XROOTD_UTILS-NOTFOUND)

if(xrootd-utils_FIND_REQUIRED AND XROOTD-NOTFOUND)
  message(FATAL_ERROR "XRootD package not found")
endif()

if(xrootd-utils_FIND_VERSION_EXACT)
  if(NOT XROOTD-VERSION VERSION_EQUAL xrootd-utils_FIND_VERSION)
    message(FATAL_ERROR "XRootD package version: " ${xrootd-utils_FIND_VERSION}
                        " not found")
  endif()
else()
  if(XROOTD-VERSION VERSION_LESS xrootd-utils_FIND_VERSION)
    message(FATAL_ERROR "Require at least XRootD version "
                        ${xrootd-utils_FIND_VERSION})
  endif()
endif()

print_message(STATUS "XRootD version: ${XROOTD-VERSION}")
