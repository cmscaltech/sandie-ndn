CMAKE_MINIMUM_REQUIRED(VERSION 3.18)

if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type")
endif()

PROJECT(NDNc C CXX)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

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
find_package(nlohmann_json 3.9.1 REQUIRED)

SET(CMAKE_CXX_STANDARD 17)

ADD_DEFINITIONS(-D_GNU_SOURCE)
ADD_COMPILE_OPTIONS(-g -Wall -Wextra -Wpedantic)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})

# compile ndnc.so
# ADD_LIBRARY(ndnc SHARED face lp mgmt)
# SET_TARGET_PROPERTIES(ndnc PROPERTIES LINKER_LANGUAGE CXX)

add_subdirectory(logger)

# compile ndncping apps
ADD_EXECUTABLE(ndncping-client 
                app/ping/client/main.cpp
                app/ping/client/ping-client.cpp
                face/face.cpp
                face/packet-handler.cpp
                pipeline/pipeline-interests-fixed.cpp
                mgmt/client.cpp)

TARGET_LINK_LIBRARIES(ndncping-client LINK_PUBLIC ${Boost_LIBRARIES} curl)
TARGET_LINK_LIBRARIES(ndncping-client PRIVATE nlohmann_json::nlohmann_json)
TARGET_LINK_LIBRARIES(ndncping-client PRIVATE Threads::Threads)
TARGET_LINK_LIBRARIES(ndncping-client PRIVATE logger)
SET_TARGET_PROPERTIES(ndncping-client PROPERTIES LINKER_LANGUAGE CXX)

ADD_EXECUTABLE(ndncping-server 
                app/ping/server/main.cpp
                app/ping/server/ping-server.cpp
                face/face.cpp
                face/packet-handler.cpp
                mgmt/client.cpp)

TARGET_LINK_LIBRARIES(ndncping-server LINK_PUBLIC ${Boost_LIBRARIES} curl)
TARGET_LINK_LIBRARIES(ndncping-server PRIVATE nlohmann_json::nlohmann_json)
TARGET_LINK_LIBRARIES(ndncping-server PRIVATE Threads::Threads)
TARGET_LINK_LIBRARIES(ndncping-server PRIVATE logger)
SET_TARGET_PROPERTIES(ndncping-server PROPERTIES LINKER_LANGUAGE CXX)

# compile ndncft apps
ADD_EXECUTABLE(ndncft-client
                app/benchmark/client/main.cpp
                app/benchmark/client/ft-client.cpp
                face/face.cpp
                face/packet-handler.cpp
                pipeline/pipeline-interests-fixed.cpp
                mgmt/client.cpp
                pipeline/pipeline-interests-aimd.cpp)

TARGET_LINK_LIBRARIES(ndncft-client LINK_PUBLIC ${Boost_LIBRARIES} curl)
TARGET_LINK_LIBRARIES(ndncft-client PRIVATE nlohmann_json::nlohmann_json)
TARGET_LINK_LIBRARIES(ndncft-client PRIVATE Threads::Threads)
TARGET_LINK_LIBRARIES(ndncft-client PRIVATE logger)
SET_TARGET_PROPERTIES(ndncft-client PROPERTIES LINKER_LANGUAGE CXX)

ADD_EXECUTABLE(ndncft-server
                app/benchmark/server/main.cpp
                app/benchmark/server/ft-server.cpp
                face/face.cpp
                face/packet-handler.cpp
                mgmt/client.cpp)

TARGET_LINK_LIBRARIES(ndncft-server LINK_PUBLIC ${Boost_LIBRARIES} curl)
TARGET_LINK_LIBRARIES(ndncft-server PRIVATE nlohmann_json::nlohmann_json)
TARGET_LINK_LIBRARIES(ndncft-server PRIVATE Threads::Threads)
TARGET_LINK_LIBRARIES(ndncft-server PRIVATE logger)
SET_TARGET_PROPERTIES(ndncft-server PROPERTIES LINKER_LANGUAGE CXX)
