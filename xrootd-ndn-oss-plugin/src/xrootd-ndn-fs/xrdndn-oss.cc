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

#include <XrdOuc/XrdOucTrace.hh>
#include <XrdVersion.hh>

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
                               const char *config_fn, const char *) {
    return (XrdNdnSS.Init(Logger, config_fn) ? 0 : (XrdOss *)&XrdNdnSS);
}
}

/*****************************************************************************/
/*         F i l e   S y s t e m   O b j e c t   I n t e r f a c e s         */
/*****************************************************************************/
int XrdNdnOss::Init(XrdSysLogger *lp, const char *) {
    m_eDest = &OssEroute;
    m_eDest->logger(lp);

    m_eDest->Say("Copyright © 2018 California Institute of Technology\n"
                 "Author: Catalin Iordache <catalin.iordache@cern.ch>");
    m_eDest->Say("Named Data Networking based Storage System initialized.");

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
    return (XrdNdnFsFile *)new XrdNdnFsFile(this);
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