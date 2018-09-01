/**************************************************************************
 * Named Data Networking plugin for xrootd                                *
 * Copyright © 2018 California Institute of Technology                    *
 *                                                                        *
 * Author: Catalin Iordache <catalin.iordache@cern.ch>                    *
 *                                                                        *
 * This program is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>. *
 **************************************************************************/

#include <XrdOuc/XrdOucTrace.hh>
#include <iostream>

#include "xrdndndfs.hh"

/**************************************************************************/
/*       O S   D i r e c t o r y   H a n d l i n g   I n t e r f a c e    */
/**************************************************************************/
#ifndef S_IAMB
#define S_IAMB 0x1FF
#endif

/**************************************************************************/
/*                  E r r o r   R o u t i n g   O b j e c t               */
/**************************************************************************/
namespace xrdndndfs {
XrdSysError OssEroute(0, "xrdndndfs_");
XrdOucTrace OssTrace(&OssEroute);
static XrdNdnDfsSys XrdNdnDfsSS;
} // namespace xrdndndfs

using namespace xrdndndfs;

/**************************************************************************/
/*                      G e t F i l e S y s t e m                         */
/**************************************************************************/
extern "C" {
XrdOss *XrdOssGetStorageSystem(XrdOss *native_oss, XrdSysLogger *Logger,
                               const char *config_fn, const char *parms) {
    return (XrdNdnDfsSS.Init(Logger, config_fn) ? 0 : (XrdOss *)&XrdNdnDfsSS);
}
}

/**************************************************************************/
/*        F i l e   S y s t e m   O b j e c t   I n t e r f a c e s       */
/**************************************************************************/
/*                                  I n i t                               */
/**************************************************************************/
int XrdNdnDfsSys::Init(XrdSysLogger *lp, const char *config_fn) {
    m_eDest = &OssEroute;
    m_eDest->logger(lp);

    m_eDest->Say("Copyright © 2018 California Institute of Technology\n"
                 "Author: Catalin Iordache <catalin.iordache@cern.ch>");
    m_eDest->Say("Named Data Networking based Storage System initialized.");
    return 0;
}