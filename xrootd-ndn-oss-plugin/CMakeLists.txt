cmake_minimum_required(VERSION 3.5)

# CMake should use the current exported location of g++
add_definitions(-DCMAKE_CXX_COMPILER=g++)
add_definitions(-D_GNU_SOURCE)

# The sources are compiled with at least GCC 5.3.0
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.3.0)
  message(FATAL_ERROR "Require at least GCC 5.3.0")
endif()

# Set path to find helper files for cmake
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Search for required libraries to compile the sources
find_package(Boost 1.58 REQUIRED
             COMPONENTS system
                        log
                        thread
                        program_options)
find_package(ndn-cxx 0.6.2 REQUIRED)
find_package(Threads)
find_package(xrootd-utils)

# Use C++14 to compile
set(CMAKE_CXX_STANDARD 14)

add_compile_options(-g -Wall -Wextra -Wpedantic -O0)
add_definitions(-DBOOST_LOG_DYN_LINK)

set(XRDNDN_PRODUCER_VERSION_MAJOR 0)
set(XRDNDN_PRODUCER_VERSION_MINOR 1)
set(XRDNDN_PRODUCER_VERSION_PATCH 4)

set(XRDNDN_CONSUMER_VERSION_MAJOR 0)
set(XRDNDN_CONSUMER_VERSION_MINOR 1)
set(XRDNDN_CONSUMER_VERSION_PATCH 4)

# Compile libXrdNdnFS.so
add_library(XrdNdnFS
            MODULE
            src/xrootd-ndn-fs/xrdndn-oss.cc
            src/xrootd-ndn-fs/xrdndn-oss-dir.cc
            src/xrootd-ndn-fs/xrdndn-oss-file.cc
            src/xrdndn-consumer/xrdndn-consumer.cc
            src/xrdndn-consumer/xrdndn-data-fetcher.cc
            src/xrdndn-consumer/xrdndn-pipeline.cc)

target_link_libraries(XrdNdnFS
                      Boost::system
                      Boost::log
                      ${NDN_CXX_LIB}
                      ${CMAKE_THREAD_LIBS_INIT}
                      ${XROOTD_UTILS})

target_include_directories(XrdNdnFS SYSTEM PUBLIC ${XROOTD_INCLUDES})

execute_process(COMMAND git describe --abbrev=8 --always
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE)

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
  set(XRDNDN_CONSUMER_VERSION_BUILD_STRING
      "${XRDNDN_CONSUMER_VERSION_STRING}-${GIT_VERSION}")
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
                      ${Boost_LIBRARIES}
                      ${NDN_CXX_LIB}
                      ${CMAKE_THREAD_LIBS_INIT})

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
  set(XRDNDN_PRODUCER_VERSION_BUILD_STRING
      "${XRDNDN_PRODUCER_VERSION_STRING}-${GIT_VERSION}")
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
                      ${Boost_LIBRARIES}
                      ${NDN_CXX_LIB}
                      ${CMAKE_THREAD_LIBS_INIT})

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
