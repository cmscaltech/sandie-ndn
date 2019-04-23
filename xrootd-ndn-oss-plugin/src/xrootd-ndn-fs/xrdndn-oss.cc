/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018-2019 California Institute of Technology                   *
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

#include <sstream>
#include <vector>

#include <XrdOuc/XrdOucTrace.hh>
#include <XrdVersion.hh>

#include "xrdndn-fs-version.hh"
#include "xrdndn-oss-dir.hh"
#include "xrdndn-oss-file.hh"
#include "xrdndn-oss.hh"

/*****************************************************************************/
/*       O S   D i r e c t o r y   H a n d l i n g   I n t e r f a c e       */
/*****************************************************************************/
#ifndef S_IAMB
#define S_IAMB 0x1FF
#endif

/*****************************************************************************/
/*                  E r r o r   R o u t i n g   O b j e c t                  */
/*****************************************************************************/
namespace xrdndnfs {
XrdSysError OssEroute(0, "xrdndnfs_");
XrdOucTrace OssTrace(&OssEroute);
static XrdNdnOss XrdNdnSS;
} // namespace xrdndnfs

using namespace xrdndnfs;

/*****************************************************************************/
/*                       G e t F i l e S y s t e m                           */
/*****************************************************************************/
extern "C" {
XrdOss *XrdOssGetStorageSystem(XrdOss *, XrdSysLogger *Logger,
                               const char *config_fn, const char *parms) {
    if (XrdNdnSS.Init(Logger, config_fn) != 0)
        return 0;

    XrdNdnSS.m_eDest->Say(
        "++++++ Configuring Named Data Networking Storage System. . .");

    if (parms && strlen(parms) != 0)
        XrdNdnSS.Configure(parms);

    XrdNdnSS.m_eDest->Say(
        "       ofs NDN Consumer pipeline size: ",
        std::to_string(XrdNdnSS.m_consumerOptions.pipelineSize).c_str());
    XrdNdnSS.m_eDest->Say(
        "       ofs NDN Consumer interest lifetime: ",
        std::to_string(XrdNdnSS.m_consumerOptions.interestLifetime).c_str());
    XrdNdnSS.m_eDest->Say("       ofs NDN Consumer log level: ",
                          XrdNdnSS.m_consumerOptions.logLevel.c_str());
    XrdNdnSS.m_eDest->Say(
        "------ Named Data Networking Storage System configuration completed.");
    return ((XrdOss *)&XrdNdnSS);
}
}

void XrdNdnOss::Configure(const char *parms) {
    std::string sparms(parms);
    { // Remove comments from params line in config file
        std::size_t foundComment = sparms.find("#");
        if (foundComment != std::string::npos)
            sparms = sparms.substr(0, foundComment);
    }

    if (sparms.empty())
        return;

    std::istringstream iss(sparms);
    std::vector<std::string> vparms(std::istream_iterator<std::string>{iss},
                                    std::istream_iterator<std::string>());

    if (vparms.size() % 2 != 0) {
        m_eDest->Emsg("Config", "the list of parameters in config file is not "
                                "even. The costum parameters will be ignored");
        return;
    }

    auto getIntFromParams = [&](std::string key, int &ret) {
        for (auto it = vparms.begin(); it != vparms.end(); ++it) {
            if (key.compare(*it) != 0)
                continue;
            try {
                ret = std::stoi(*++it);
                return true;
            } catch (const std::exception &e) {
                m_eDest->Emsg("Config", e.what(),
                              "while getting int value from params list");
                return false;
            }
        }
        return false;
    };

    auto getLogLevelFromParams = [&](std::string &logLevel) {
        std::string key("loglevel");
        for (auto it = vparms.begin(); it != vparms.end(); ++it) {
            if (key.compare(*it) != 0)
                continue;
            ++it;
            logLevel = *it;
            std::transform(logLevel.begin(), logLevel.end(), logLevel.begin(),
                           ::toupper);
            if ((logLevel).compare("TRACE") != 0 &&
                (logLevel).compare("DEBUG") != 0 &&
                (logLevel).compare("INFO") != 0 &&
                (logLevel).compare("WARN") != 0 &&
                (logLevel).compare("ERROR") != 0 &&
                (logLevel).compare("FATAL") != 0 &&
                (logLevel).compare("NONE") != 0) {
                return false;
            }
            return true;
        }
        return false;
    };

    {
        int pipesize;
        if (getIntFromParams("pipelinesize", pipesize)) {
            if (pipesize < XRDNDN_MINPIPELINESZ ||
                pipesize > XRDNDN_MAXPIPELINESZ) {
                m_eDest->Emsg(
                    "Config",
                    std::string(
                        "Pipeline size must be between " +
                        std::to_string(XRDNDN_MINPIPELINESZ) + " and " +
                        std::to_string(XRDNDN_MAXPIPELINESZ) +
                        ". The pipeline size will be set to default value")
                        .c_str());
            } else {
                m_consumerOptions.pipelineSize = pipesize;
            }
        }
    }

    {
        int interestLifetime;
        if (getIntFromParams("interestlifetime", interestLifetime)) {
            if (interestLifetime < XRDNDN_MININTEREST_LIFETIME ||
                interestLifetime > XRDNDN_MAXINTEREST_LIFETIME) {
                m_eDest->Emsg(
                    "Config",
                    std::string(
                        "Interest lifetime must be between " +
                        std::to_string(XRDNDN_MININTEREST_LIFETIME) + " and " +
                        std::to_string(XRDNDN_MAXINTEREST_LIFETIME) +
                        ". The Interest lifetime will be set to default value")
                        .c_str());
            } else {
                m_consumerOptions.interestLifetime = interestLifetime;
            }
        }
    }

    {
        std::string logLevel;
        if (getLogLevelFromParams(logLevel)) {
            m_consumerOptions.logLevel = logLevel;
        } else {
            m_eDest->Emsg("Config",
                          "The log level must be one of: TRACE, DEBUG, INFO, "
                          "WARN, ERROR, FATAL, NONE. The log level will be set "
                          "to default value INFO");
        }
    }
}

/*****************************************************************************/
/*         F i l e   S y s t e m   O b j e c t   I n t e r f a c e s         */
/*****************************************************************************/
int XrdNdnOss::Init(XrdSysLogger *lp, const char *) {
    m_eDest = &OssEroute;
    m_eDest->logger(lp);

    m_eDest->Say("Copyright © 2018-2019 California Institute of Technology\n"
                 "Author: Catalin Iordache <catalin.iordache@cern.ch>");
    m_eDest->Say("Named Data Networking based Storage System build ",
                 XRDNDN_FS_VERSION_BUILD_STRING, " initialized.");

    return 0;
}

void XrdNdnOss::Say(const char *msg, const char *x, const char *y,
                    const char *z) {
    m_eDest->Say(msg, x, y, z);
}

// Taken from https://github.com/opensciencegrid/xrootd-hdfs
int XrdNdnOss::Emsg(const char *pfx,      // Message prefix value
                    XrdOucErrInfo &einfo, // Place to put text & error code
                    int ecode,            // The error code
                    const char *op,       // Operation being performed
                    const char *target)   // The target (e.g., fname)
{
    char *etext, buffer[XrdOucEI::Max_Error_Len], unkbuff[64];

    // Get the reason for the error
    if (ecode < 0)
        ecode = -ecode;
    if (!(etext = strerror(ecode))) {
        sprintf(unkbuff, "reason unknown (%d)", ecode);
        etext = unkbuff;
    }

    // Format the error message
    snprintf(buffer, sizeof(buffer), "Unable to %s %s; %s", op, target, etext);

#ifndef NODEBUG // Print it out if debugging is enabled
    OssEroute.Emsg(pfx, buffer);
#endif

    // Place the error message in the error object and return
    einfo.setErrInfo(ecode, buffer);

    if (errno != 0)
        return (errno > 0) ? -errno : errno;
    return -1;
}

XrdOssDF *XrdNdnOss::newDir(const char *tident) {
    return (XrdNdnOssDir *)new XrdNdnOssDir(tident);
}

XrdOssDF *XrdNdnOss::newFile(const char *) {
    return (XrdNdnOssFile *)new XrdNdnOssFile(this);
}

int XrdNdnOss::Chmod(const char *, mode_t, XrdOucEnv *) { return -ENOTSUP; }

int XrdNdnOss::Create(const char *, const char *, mode_t, XrdOucEnv &, int) {
    return -ENOTSUP;
}

int XrdNdnOss::Mkdir(const char *, mode_t, int, XrdOucEnv *) {
    return -ENOTSUP;
}

int XrdNdnOss::Remdir(const char *, int, XrdOucEnv *) { return -ENOTSUP; }

int XrdNdnOss::Rename(const char *, const char *, XrdOucEnv *, XrdOucEnv *) {
    return -ENOTSUP;
}

int XrdNdnOss::Stat(const char *, struct stat *, int, XrdOucEnv *) {
    return -ENOTSUP;
}

int XrdNdnOss::Truncate(const char *, unsigned long long, XrdOucEnv *) {
    return -ENOTSUP;
}

int XrdNdnOss::Unlink(const char *, int, XrdOucEnv *) { return -ENOTSUP; }

XrdVERSIONINFO(XrdOssGetStorageSystem, "xrdndnfs")