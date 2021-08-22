function(print_message mode VALUE)
  if(NOT ndn-cxx_FIND_QUIETLY)
    message(${mode} ${VALUE})
  endif()
endfunction()


# search for ndn-cxx includes
print_message(STATUS "Looking for ndn-cxx/version.hpp")
find_path(NDN_CXX_INCLUDES version.hpp
          HINTS "/usr/" "/usr/local/"
          PATH_SUFFIXES "include/ndn-cxx")

if(NDN_CXX_INCLUDES-NOTFOUND)
  print_message(STATUS "Looking for ndn-cxx/version.hpp - not found")
  set(NDN_CXX-NOTFOUND TRUE)
else()
  # parse ndn-cxx version
  file(READ "${NDN_CXX_INCLUDES}/version.hpp" ndn-cxx-version-content)
  string(REGEX MATCH "NDN_CXX_VERSION_STRING [\"]([0-9]+)\\.([0-9]+)\\.([0-9]+)[\"]" NDN_CXX-VERSION ${ndn-cxx-version-content})
  string(REPLACE "NDN_CXX_VERSION_STRING " "" NDN_CXX-VERSION ${NDN_CXX-VERSION})
  string(REPLACE "\"" "" NDN_CXX-VERSION ${NDN_CXX-VERSION})

  # set ndn-cxx include path
  get_filename_component(NDN_CXX_INCLUDES "${NDN_CXX_INCLUDES}/../" ABSOLUTE)
  print_message(STATUS
                "Looking for ndn-cxx/version.hpp - found: ${NDN_CXX_INCLUDES}")
  set(NDN_CXX-FOUND TRUE)
endif(NDN_CXX_INCLUDES-NOTFOUND)


# search for ndn-cxx libs
print_message(STATUS "Looking for libndn-cxx")
find_library(NDN_CXX_LIB ndn-cxx HINTS "/usr/" "/usr/local/")

if(NDN_CXX_LIB-NOTFOUND)
  print_message(STATUS "Looking for libndn-cxx - not found")
  set(NDN_CXX-NOTFOUND TRUE)
else()
  print_message(STATUS "Looking for libndn-cxx - found: ${NDN_CXX_LIB}")
  set(NDN_CXX-FOUND TRUE)
endif(NDN_CXX_LIB-NOTFOUND)

if(ndn-cxx_FIND_REQUIRED AND NDN_CXX-NOTFOUND)
  message(FATAL_ERROR "ndn-cxx package not found")
endif()

if(ndn-cxx_FIND_VERSION_EXACT)
  if(NOT NDN_CXX-VERSION VERSION_EQUAL ndn-cxx_FIND_VERSION)
    message(FATAL_ERROR "ndn-cxx package version: " ${ndn-cxx_FIND_VERSION} " not found")
  endif()
else()
  if(NDN_CXX-VERSION VERSION_LESS ndn-cxx_FIND_VERSION)
    message(FATAL_ERROR "Require at least ndn-cxx version " ${ndn-cxx_FIND_VERSION})
  endif()
endif()

print_message(STATUS "ndn-cxx version: ${NDN_CXX-VERSION}")
