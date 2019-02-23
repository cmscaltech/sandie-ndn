find_path(XROOTD_INCLUDES XrdVersion.hh
    HINTS "/usr/" "/opt/xrootd"
    PATH_SUFFIXES "include/xrootd"
    PATHS "/opt/xrootd")

find_library(XROOTD_UTILS XrdUtils
    HINTS "/usr/" "/opt/xrootd"
    PATH_SUFFIXES "include/xrootd"
    PATHS "/opt/xrootd")