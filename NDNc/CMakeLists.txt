CMAKE_MINIMUM_REQUIRED(VERSION 3.18)

if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type")
endif()

PROJECT(NDNc C CXX)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    FIND_PACKAGE(memif 4.0 REQUIRED)
    INCLUDE_DIRECTORIES(SYSTEM "${MEMIF_INCLUDES}")
    LINK_LIBRARIES("${MEMIF_LIB}")
endif()

FIND_PACKAGE(ndn-cxx 0.7.1 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM "${NDN_CXX_INCLUDES}")
LINK_LIBRARIES("${NDN_CXX_LIB}")

FIND_PACKAGE(Boost 1.74 COMPONENTS program_options system filesystem REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

FIND_PACKAGE(CURL REQUIRED)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(nlohmann_json 3.9.1 REQUIRED)
FIND_PACKAGE(xrootd-utils 5.0.0 REQUIRED)

SET(CMAKE_CXX_STANDARD 17)

ADD_DEFINITIONS(-D_GNU_SOURCE)
ADD_COMPILE_OPTIONS(-g -Wall -Wextra -Wpedantic -fPIC)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

EXECUTE_PROCESS(COMMAND git describe --abbrev=8 --always --tags
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE)

ADD_SUBDIRECTORY(logger)

# compile ndnc library
ADD_LIBRARY(ndnc SHARED
                face/memif.cpp
                face/face.cpp
                face/packet-handler.cpp
                congestion-control/pipeline-interests-fixed.cpp
                congestion-control/pipeline-interests-aimd.cpp
                mgmt/client.cpp
                utils
                lib/posix/consumer.cpp
                lib/posix/file.cpp
                lib/posix/dir.cpp)

TARGET_LINK_LIBRARIES(ndnc PRIVATE logger)
TARGET_LINK_LIBRARIES(ndnc PRIVATE curl)
TARGET_LINK_LIBRARIES(ndnc PRIVATE nlohmann_json::nlohmann_json)
SET_TARGET_PROPERTIES(ndnc PROPERTIES LINKER_LANGUAGE CXX)

# compile ndncping apps
ADD_EXECUTABLE(ndncping-client 
                app/ping/client/main.cpp
                app/ping/client/ping-client.cpp)

TARGET_LINK_LIBRARIES(ndncping-client LINK_PUBLIC ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(ndncping-client PRIVATE nlohmann_json::nlohmann_json)
TARGET_LINK_LIBRARIES(ndncping-client PRIVATE Threads::Threads)
TARGET_LINK_LIBRARIES(ndncping-client PRIVATE ndnc)
SET_TARGET_PROPERTIES(ndncping-client PROPERTIES LINKER_LANGUAGE CXX)

ADD_EXECUTABLE(ndncping-server 
                app/ping/server/main.cpp
                app/ping/server/ping-server.cpp)

TARGET_LINK_LIBRARIES(ndncping-server LINK_PUBLIC ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(ndncping-server PRIVATE nlohmann_json::nlohmann_json)
TARGET_LINK_LIBRARIES(ndncping-server PRIVATE Threads::Threads)
TARGET_LINK_LIBRARIES(ndncping-server PRIVATE ndnc)
SET_TARGET_PROPERTIES(ndncping-server PROPERTIES LINKER_LANGUAGE CXX)

# compile ndncft apps
ADD_EXECUTABLE(ndncft-client
                app/file-transfer/client/main.cpp
                app/file-transfer/client/ft-client.cpp)

TARGET_LINK_LIBRARIES(ndncft-client LINK_PUBLIC ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(ndncft-client PRIVATE Threads::Threads)
TARGET_LINK_LIBRARIES(ndncft-client PRIVATE ndnc)
TARGET_LINK_LIBRARIES(ndncft-client PRIVATE curl)
SET_TARGET_PROPERTIES(ndncft-client PROPERTIES LINKER_LANGUAGE CXX)

ADD_EXECUTABLE(ndncft-server
                app/file-transfer/server/main.cpp
                app/file-transfer/server/ft-server.cpp)

TARGET_LINK_LIBRARIES(ndncft-server LINK_PUBLIC ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(ndncft-server PRIVATE nlohmann_json::nlohmann_json)
TARGET_LINK_LIBRARIES(ndncft-server PRIVATE Threads::Threads)
TARGET_LINK_LIBRARIES(ndncft-server PRIVATE ndnc)
SET_TARGET_PROPERTIES(ndncft-server PROPERTIES LINKER_LANGUAGE CXX)

# compile XrdNdnOss library
SET(XRDNDNOSS_VERSION_MAJOR 0)
SET(XRDNDNOSS_VERSION_MINOR 1)
SET(XRDNDNOSS_VERSION_PATCH 0)

MATH(
  EXPR XRDNDNOSS_VERSION
  ${XRDNDNOSS_VERSION_MAJOR}*1000000+${XRDNDNOSS_VERSION_MINOR}*1000+${XRDNDNOSS_VERSION_PATCH}
  )
SET(
  XRDNDNOSS_VERSION_STRING
  "${XRDNDNOSS_VERSION_MAJOR}.${XRDNDNOSS_VERSION_MINOR}.${XRDNDNOSS_VERSION_PATCH}"
  )

if(GIT_VERSION STREQUAL "")
  SET(XRDNDNOSS_VERSION_BUILD_STRING "${XRDNDNOSS_VERSION_STRING}")
else()
  SET(XRDNDNOSS_VERSION_BUILD_STRING "${GIT_VERSION}")
endif()

configure_file("${PROJECT_SOURCE_DIR}/lib/xrdndnoss/xrd-ndn-oss-version.hpp.in"
               "${PROJECT_BINARY_DIR}/include/xrd-ndn-oss-version.hpp")
INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}/include/")

ADD_LIBRARY(XrdNdnOss SHARED
            lib/xrdndnoss/xrd-ndn-oss.cpp
            lib/xrdndnoss/xrd-ndn-oss-file.cpp
            lib/xrdndnoss/xrd-ndn-oss-dir.cpp)

TARGET_LINK_LIBRARIES(XrdNdnOss LINK_PUBLIC ${XROOTD_UTILS} ndnc)
TARGET_INCLUDE_DIRECTORIES(XrdNdnOss SYSTEM PUBLIC ${XROOTD_INCLUDES})

# install
SET(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

if(NOT DEFINED LIB_INSTALL_DIR)
  SET(LIB_INSTALL_DIR "/usr/lib64")
endif()

INSTALL(TARGETS ndnc XrdNdnOss
     RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT libraries
     LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT libraries)
