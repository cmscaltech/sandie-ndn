/*
 * N-DISE: NDN for Data Intensive Science Experiments
 * Author: Catalin Iordache <catalin.iordache@cern.ch>
 *
 * MIT License
 *
 * Copyright (c) 2022 California Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <sstream>

#include <XrdOuc/XrdOucTrace.hh>
#include <XrdVersion.hh>

#include "xrd-ndn-oss-dir.hpp"
#include "xrd-ndn-oss-file.hpp"
#include "xrd-ndn-oss-version.hpp"
#include "xrd-ndn-oss.hpp"

/**
 * @brief OS Directory Handling Interface
 *
 */
#ifndef S_IAMB
#define S_IAMB 0x1FF
#endif

namespace xrdndnofs {
XrdSysError OssEroute(0, "xrdndnoss_");
XrdOucTrace OssTrace(&OssEroute);
static XrdNdnOss XrdNdnOfs;
} // namespace xrdndnofs

using namespace xrdndnofs;

// Get FileSystem
extern "C" {
XrdOss *XrdOssGetStorageSystem(XrdOss *, XrdSysLogger *Logger,
                               const char *config_fn, const char *parms) {
    if (XrdNdnOfs.Init(Logger, config_fn) != 0) {
        XrdNdnOfs.Emsg("XrdOssGetStorageSystem", XrdNdnOfs.error_, -1,
                       "init XrdNdnOfs");
        return NULL;
    }

    return ((XrdOss *)&XrdNdnOfs);
}
}

XrdNdnOss::XrdNdnOss() : XrdOss(), consumerOptions_{} {
    this->consumer_ =
        std::make_shared<ndnc::posix::Consumer>(this->consumerOptions_);
}

XrdNdnOss::~XrdNdnOss() {
    // TODO: Need to investigate why ths is never called
    if (eDest_ != nullptr) {
        eDest_->Say("d'tor: XrdNdnOss");
    }
}

void XrdNdnOss::Say(const char *msg, const char *x = "", const char *y = "",
                    const char *z = "") {
    eDest_->Say(msg, x, y, z);
}

int XrdNdnOss::Emsg(const char *pfx, XrdOucErrInfo &einfo, int ecode,
                    const char *op, const char *target) {
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

XrdOssDF *XrdNdnOss::newDir(const char * /*tident*/) {
    if (!this->consumer_->isValid()) {
        Emsg("newDir", XrdNdnOfs.error_, -1, "null consumer");
        return nullptr;
    }

    return (XrdNdnOssDir *)new XrdNdnOssDir(this->consumer_);
}

XrdOssDF *XrdNdnOss::newFile(const char * /*tident*/) {
    if (!this->consumer_->isValid()) {
        Emsg("newFile", XrdNdnOfs.error_, -1, "null consumer");
        return nullptr;
    }

    return (XrdNdnOssFile *)new XrdNdnOssFile(this->consumer_);
}

int XrdNdnOss::Chmod(const char *, mode_t, XrdOucEnv *) {
    return -ENOTSUP;
}

int XrdNdnOss::Create(const char *, const char *, mode_t, XrdOucEnv &, int) {
    return -ENOTSUP;
}

int XrdNdnOss::Init(XrdSysLogger *lp, const char *cfn) {
    eDest_ = &OssEroute;
    eDest_->logger(lp);

    eDest_->Say("Copyright © 2022 California Institute of Technology\n"
                "Author: Catalin Iordache <catalin.iordache@cern.ch>");
    eDest_->Say("Named Data Networking storage system v",
                XRDNDNOSS_VERSION_STRING, " initialization.\n");

    if (!consumer_->isValid()) {
        return -1;
    }

    return XrdOssOK;
}

int XrdNdnOss::Mkdir(const char *, mode_t, int = 0, XrdOucEnv * = 0) {
    return 0;
}

int XrdNdnOss::Remdir(const char *, int, XrdOucEnv *) {
    return 0;
}

int XrdNdnOss::Rename(const char *, const char *, XrdOucEnv *, XrdOucEnv *) {
    return -ENOTSUP;
}

int XrdNdnOss::Stat(const char *path, struct stat *buff, int = 0,
                    XrdOucEnv * = 0) {
    auto file = std::make_shared<ndnc::posix::File>(this->consumer_);
    return file->stat(path, buff);
}

int XrdNdnOss::Truncate(const char *, unsigned long long, XrdOucEnv *) {
    return -ENOTSUP;
}

int XrdNdnOss::Unlink(const char *, int, XrdOucEnv *) {
    return -ENOTSUP;
}

XrdVERSIONINFO(XrdOssGetStorageSystem, "xrdndnoss")
