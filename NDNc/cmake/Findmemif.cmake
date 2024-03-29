function(print_message mode VALUE)
  if(NOT memif_FIND_QUIETLY)
    message(${mode} ${VALUE})
  endif()
endfunction()


# search for libmemif.h
print_message(STATUS "Looking for libmemif.h")
find_path(MEMIF_INCLUDES libmemif.h
          HINTS "/usr/" "/usr/local/"
          PATH_SUFFIXES "include")

if(MEMIF_INCLUDES-NOTFOUND)
  print_message(STATUS "Looking for libmemif.h - not found")
  set(MEMIF-NOTFOUND TRUE)
else()
  file(READ "${MEMIF_INCLUDES}/libmemif.h" memif-version-content)
  string(REGEX MATCH "LIBMEMIF_VERSION [\"]([0-9]+)\\.([0-9]+)[\"]" MEMIF-VERSION ${memif-version-content})
  string(REPLACE "LIBMEMIF_VERSION " "" MEMIF-VERSION ${MEMIF-VERSION})
  string(REPLACE "\"" "" MEMIF-VERSION ${MEMIF-VERSION})

  # set libmemif includes
  get_filename_component(MEMIF_INCLUDES "${MEMIF_INCLUDES}/../" ABSOLUTE)
  print_message(STATUS "Looking for libmemif.h - found: ${MEMIF_INCLUDES}")
  set(MEMIF-FOUND TRUE)
endif(MEMIF_INCLUDES-NOTFOUND)


# search for libmemif.so
print_message(STATUS "Looking for libmemif")
find_library(MEMIF_LIB memif HINTS "/usr/" "/usr/local/" "/usr/local/lib")

if(MEMIF_LIB-NOTFOUND)
  print_message(STATUS "Looking for libmemif - not found")
  set(MEMIF-NOTFOUND TRUE)
else()
  print_message(STATUS "Looking for libmemif - found: ${MEMIF_LIB}")
  set(MEMIF-FOUND TRUE)
endif(MEMIF_LIB-NOTFOUND)

if(memif_FIND_REQUIRED AND MEMIF-NOTFOUND)
  message(FATAL_ERROR "libmemif not found")
endif()

if(memif_FIND_VERSION_EXACT)
  if(NOT MEMIF-VERSION VERSION_EQUAL memif_FIND_VERSION)
    message(FATAL_ERROR "libmemif version: " ${memif_FIND_VERSION} " not found")
  endif()
else()
  if(MEMIF-VERSION VERSION_LESS memif_FIND_VERSION)
    message(FATAL_ERROR "Require at least libmemif version " ${memif_FIND_VERSION})
  endif()
endif()

print_message(STATUS "libmemif version: ${MEMIF-VERSION}")
