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

#include <XrdSys/XrdSysError.hh>
#include <XrdSys/XrdSysPlugin.hh>
#include <XrdVersion.hh>

XrdVERSIONINFO(XrdOssGetStorageSystem, "xrdndndfs");
XrdSysError OssErouteMain(0, "xrdndnd_main_");

// Forward declarations.
class XrdOss;
class XrdSysLogger;

static XrdOss *XrdOssGetSS(XrdOss *, XrdSysLogger *, const char *,
                           const char *);

extern "C" {
// Get an instance of a configured XrdOss object.
XrdOss *XrdOssGetStorageSystem(XrdOss *native_oss, XrdSysLogger *Logger,
                               const char *config_fn, const char *parms) {
    return XrdOssGetSS(native_oss, Logger, config_fn, parms);
}
}

// This function is called by the OFS layer to retrieve the Storage System
// object. If a plugin library has been specified, then this function will
// return the object provided by XrdOssGetStorageSystem() within the library.
//
static XrdOss *XrdOssGetSS(XrdOss *native_oss, XrdSysLogger *Logger,
                           const char *config_fn, const char *parms) {
    OssErouteMain.logger(Logger);

    // Create a plugin object. Take into account the proxy library.
    XrdSysPlugin *myLib;
    myLib = new XrdSysPlugin(&OssErouteMain, "libXrdNdnDFSReal.so");
    if (!myLib)
        return 0;

    // Get the entry point of the object creator
    XrdOss *(*ep)(XrdOss *, XrdSysLogger *, const char *, const char *);
    ep = (XrdOss * (*)(XrdOss *, XrdSysLogger *, const char *, const char *))(
        myLib->getPlugin("XrdOssGetStorageSystem"));
    if (!ep)
        return 0;

    return ep(native_oss, Logger, config_fn, parms);
}
