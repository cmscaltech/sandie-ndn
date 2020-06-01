#include <stdio.h>
#include <stdlib.h>

#include "logger.hh"

int ParseLogLevel(const char *module) {
    char envKey[32];
    snprintf(envKey, sizeof(envKey), "LOG_%s", module);
    const char *lvl = getenv(envKey);
    if (lvl == NULL) {
        lvl = getenv("LOG");
    }
    if (lvl == NULL) {
        lvl = "";
    }

    switch (lvl[0]) {
    case 'V':
        return ZF_LOG_VERBOSE;
    case 'D':
        return ZF_LOG_DEBUG;
    case 'I':
        return ZF_LOG_INFO;
    case 'W':
        return ZF_LOG_WARN;
    case 'E':
        return ZF_LOG_ERROR;
    case 'F':
        return ZF_LOG_FATAL;
    case 'N':
        return ZF_LOG_NONE;
    }

    return ZF_LOG_INFO;
}
