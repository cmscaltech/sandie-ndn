cmake_minimum_required(VERSION 3.5)

# CMake should use the current exported location of g++
add_definitions(-DCMAKE_CXX_COMPILER=g++)
add_definitions(-D_GNU_SOURCE)

# The sources are compiled with at least GCC 8.2.1
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.2.1)
  message(FATAL_ERROR "Require at least GCC 8.2.1")
endif()

# Set path to find helper files for cmake
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Search for required libraries to compile the sources
find_package(ndn-cxx 0.7.0 REQUIRED)
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_package(xrootd-utils 4.8.3 REQUIRED)
endif()
find_package(Boost 1.69 REQUIRED
             COMPONENTS system
                        log
                        thread
                        program_options)

# Use C++14 to compile
set(CMAKE_CXX_STANDARD 14)

add_compile_options(-g -Wall -Wextra -Wpedantic -O2)
add_definitions(-DBOOST_LOG_DYN_LINK)

include_directories(SYSTEM "${NDN_CXX_INCLUDES}")
link_libraries("${NDN_CXX_LIB}")

set(XRDNDN_PRODUCER_VERSION_MAJOR 0)
set(XRDNDN_PRODUCER_VERSION_MINOR 2)
set(XRDNDN_PRODUCER_VERSION_PATCH 0)

set(XRDNDN_CONSUMER_VERSION_MAJOR 0)
set(XRDNDN_CONSUMER_VERSION_MINOR 2)
set(XRDNDN_CONSUMER_VERSION_PATCH 0)

set(XRDNDN_FS_VERSION_MAJOR 0)
set(XRDNDN_FS_VERSION_MINOR 2)
set(XRDNDN_FS_VERSION_PATCH 0)

execute_process(COMMAND git describe --abbrev=8 --always --tags
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# Compile libXrdNdnFS.so
math(
  EXPR XRDNDN_FS_VERSION
  ${XRDNDN_FS_VERSION_MAJOR}*1000000+${XRDNDN_FS_VERSION_MINOR}*1000+${XRDNDN_FS_VERSION_PATCH}
  )
set(
  XRDNDN_FS_VERSION_STRING
  "${XRDNDN_FS_VERSION_MAJOR}.${XRDNDN_FS_VERSION_MINOR}.${XRDNDN_FS_VERSION_PATCH}"
  )

if(GIT_VERSION STREQUAL "")
  set(XRDNDN_FS_VERSION_BUILD_STRING "${XRDNDN_FS_VERSION_STRING}")
else()
  set(XRDNDN_FS_VERSION_BUILD_STRING "${GIT_VERSION}")
endif()

configure_file("${PROJECT_SOURCE_DIR}/src/xrootd-ndn-fs/xrdndn-fs-version.hh.in"
               "${PROJECT_BINARY_DIR}/include/xrdndn-fs-version.hh")

include_directories("${PROJECT_BINARY_DIR}/include/")

add_library(XrdNdnFS
            MODULE
            src/xrootd-ndn-fs/xrdndn-oss.cc
            src/xrootd-ndn-fs/xrdndn-oss-dir.cc
            src/xrootd-ndn-fs/xrdndn-oss-file.cc
            src/xrdndn-consumer/xrdndn-consumer.cc
            src/xrdndn-consumer/xrdndn-data-fetcher.cc
            src/xrdndn-consumer/xrdndn-pipeline.cc)

target_link_libraries(XrdNdnFS ${XROOTD_UTILS} Boost::log Boost::system)

target_include_directories(XrdNdnFS SYSTEM PUBLIC ${XROOTD_INCLUDES})

# Compile xrdndn-consumer application
math(
  EXPR XRDNDN_CONSUMER_VERSION
  ${XRDNDN_CONSUMER_VERSION_MAJOR}*1000000+${XRDNDN_CONSUMER_VERSION_MINOR}*1000+${XRDNDN_CONSUMER_VERSION_PATCH}
  )
set(
  XRDNDN_CONSUMER_VERSION_STRING
  "${XRDNDN_CONSUMER_VERSION_MAJOR}.${XRDNDN_CONSUMER_VERSION_MINOR}.${XRDNDN_CONSUMER_VERSION_PATCH}"
  )

if(GIT_VERSION STREQUAL "")
  set(XRDNDN_CONSUMER_VERSION_BUILD_STRING "${XRDNDN_CONSUMER_VERSION_STRING}")
else()
  set(XRDNDN_CONSUMER_VERSION_BUILD_STRING "${GIT_VERSION}")
endif()

configure_file(
  "${PROJECT_SOURCE_DIR}/src/xrdndn-consumer/xrdndn-consumer-version.hh.in"
  "${PROJECT_BINARY_DIR}/include/xrdndn-consumer-version.hh")

include_directories("${PROJECT_BINARY_DIR}/include/")

add_executable(xrdndn-consumer
               src/xrdndn-consumer/xrdndn-consumer-main.cc
               src/xrdndn-consumer/xrdndn-consumer.cc
               src/xrdndn-consumer/xrdndn-data-fetcher.cc
               src/xrdndn-consumer/xrdndn-pipeline.cc)

target_link_libraries(xrdndn-consumer
                      Boost::log
                      Boost::program_options
                      Boost::system)

# Compile xrdndn-producer application
math(
  EXPR XRDNDN_PRODUCER_VERSION
  ${XRDNDN_PRODUCER_VERSION_MAJOR}*1000000+${XRDNDN_PRODUCER_VERSION_MINOR}*1000+${XRDNDN_PRODUCER_VERSION_PATCH}
  )
set(
  XRDNDN_PRODUCER_VERSION_STRING
  "${XRDNDN_PRODUCER_VERSION_MAJOR}.${XRDNDN_PRODUCER_VERSION_MINOR}.${XRDNDN_PRODUCER_VERSION_PATCH}"
  )

if(GIT_VERSION STREQUAL "")
  set(XRDNDN_PRODUCER_VERSION_BUILD_STRING "${XRDNDN_PRODUCER_VERSION_STRING}")
else()
  set(XRDNDN_PRODUCER_VERSION_BUILD_STRING "${GIT_VERSION}")
endif()

configure_file(
  "${PROJECT_SOURCE_DIR}/src/xrdndn-producer/xrdndn-producer-version.hh.in"
  "${PROJECT_BINARY_DIR}/include/xrdndn-producer-version.hh")
include_directories("${PROJECT_BINARY_DIR}/include/")

add_executable(xrdndn-producer
               src/xrdndn-producer/xrdndn-producer-main.cc
               src/xrdndn-producer/xrdndn-producer.cc
               src/xrdndn-producer/xrdndn-interest-manager.cc
               src/xrdndn-producer/xrdndn-file-handler.cc
               src/xrdndn-producer/xrdndn-packager.cc)

target_link_libraries(xrdndn-producer
                      Boost::log
                      Boost::program_options
                      Boost::system)

# Install
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

if(NOT DEFINED LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR "/usr/lib64")
endif()

install(TARGETS XrdNdnFS LIBRARY DESTINATION ${LIB_INSTALL_DIR} OPTIONAL)

if(NOT DEFINED BIN_INSTALL_DIR)
  set(BIN_INSTALL_DIR "/usr/bin")
endif()

install(TARGETS xrdndn-producer RUNTIME DESTINATION ${BIN_INSTALL_DIR} OPTIONAL)

install(TARGETS xrdndn-consumer RUNTIME DESTINATION ${BIN_INSTALL_DIR} OPTIONAL)
