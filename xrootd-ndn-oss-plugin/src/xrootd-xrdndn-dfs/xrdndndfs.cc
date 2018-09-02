/******************************************************************************
 * Named Data Networking plugin for xrootd                                    *
 * Copyright © 2018 California Institute of Technology                        *
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
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include "../xrdndn-consumer.hh"
#include "xrdndndfs.hh"

/*****************************************************************************/
/*       O S   D i r e c t o r y   H a n d l i n g   I n t e r f a c e       */
/*****************************************************************************/
#ifndef S_IAMB
#define S_IAMB 0x1FF
#endif

/*****************************************************************************/
/*                  E r r o r   R o u t i n g   O b j e c t                  */
/*****************************************************************************/
namespace xrdndndfs {
XrdSysError OssEroute(0, "xrdndndfs_");
XrdOucTrace OssTrace(&OssEroute);
static XrdNdnDfsSys XrdNdnDfsSS;
} // namespace xrdndndfs

using namespace xrdndndfs;

/*****************************************************************************/
/*                       G e t F i l e S y s t e m                           */
/*****************************************************************************/
extern "C" {
XrdOss *XrdOssGetStorageSystem(XrdOss *native_oss, XrdSysLogger *Logger,
                               const char *config_fn, const char *parms) {
    return (XrdNdnDfsSS.Init(Logger, config_fn) ? 0 : (XrdOss *)&XrdNdnDfsSS);
}
}

/*****************************************************************************/
/*                F i l e   O b j e c t   I n t e r f a c e s                */
/*****************************************************************************/
XrdNdnDfsFile::XrdNdnDfsFile(const char *tident) {}

XrdNdnDfsFile::~XrdNdnDfsFile() {}

/*****************************************************************************/
/*                                  O p e n                                  */
/*****************************************************************************/
/* Over NDN the file will be opened for reading only, thus only path parameter
 * is considered.*/
int XrdNdnDfsFile::Open(const char *path, int openMode, mode_t createMode,
                        XrdOucEnv &client) {
    // XrdNdnDfsSS.Say("Open file: ", path);

    filePath = std::string(path);

    xrdndn::Consumer consumer;
    int retOpen = consumer.Open(filePath);
    return retOpen;
}

/*****************************************************************************/
/*                                F s t a t                                  */
/*****************************************************************************/
int XrdNdnDfsFile::Fstat(struct stat *buf) {
    // XrdNdnDfsSS.Say("Fstat on file: ", filePath.c_str());

    xrdndn::Consumer consumer;
    return consumer.Fstat(buf, filePath);
}

/*****************************************************************************/
/*                                  R e a d                                  */
/*****************************************************************************/
ssize_t XrdNdnDfsFile::Read(void *buff, off_t offset, size_t blen) {
    // XrdNdnDfsSS.Say("Read file: ", filePath.c_str());

    xrdndn::Consumer consumer;
    int retRead = consumer.Read(buff, offset, blen, filePath);
    return retRead;
}

ssize_t XrdNdnDfsFile::Read(off_t, size_t) { return XrdOssOK; }

ssize_t XrdNdnDfsFile::ReadRaw(void *buff, off_t offset, size_t blen) {
    return Read(buff, offset, blen);
}

int XrdNdnDfsFile::Read(XrdSfsAio *aiop) {
    aiop->Result = this->Read((void *)aiop->sfsAio.aio_buf,
                              aiop->sfsAio.aio_offset, aiop->sfsAio.aio_nbytes);
    aiop->doneRead();
    return 0;
}

/*****************************************************************************/
/*                                 C l o s e                                 */
/*****************************************************************************/
int XrdNdnDfsFile::Close(long long *retsz) {
    // XrdNdnDfsSS.Say("Close file: ", filePath.c_str());
    xrdndn::Consumer consumer;
    int retClose = consumer.Close(filePath);
    filePath.clear();
    return retClose;
}

/*****************************************************************************/
/*         F i l e   S y s t e m   O b j e c t   I n t e r f a c e s         */
/*****************************************************************************/
int XrdNdnDfsSys::Init(XrdSysLogger *lp, const char *config_fn) {
    m_eDest = &OssEroute;
    m_eDest->logger(lp);

    m_eDest->Say("Copyright © 2018 California Institute of Technology\n"
                 "Author: Catalin Iordache <catalin.iordache@cern.ch>");
    m_eDest->Say("Named Data Networking based Storage System initialized.");
    return 0;
}

void XrdNdnDfsSys::Say(const char *msg, const char *x, const char *y,
                       const char *z) {
    m_eDest->Say(msg, x, y, z);
}

// Taken from https://github.com/opensciencegrid/xrootd-hdfs
int XrdNdnDfsSys::Emsg(const char *pfx,      // Message prefix value
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