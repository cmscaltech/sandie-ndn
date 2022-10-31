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

#include <signal.h>
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

static void sigint_handler(sig_atomic_t signum) {
    exit(signum);
}
}; // namespace xrdndnofs

using namespace xrdndnofs;

// Get the FileSystem
extern "C" {
XrdOss *XrdOssGetStorageSystem(XrdOss *, XrdSysLogger *Logger,
                               const char *config_fn, const char *parms) {

    if (parms && strlen(parms) != 0) {
        if (!XrdNdnOfs.Config(parms)) {
            return NULL;
        }
    }

    if (XrdNdnOfs.Init(Logger, config_fn) != 0) {
        XrdNdnOfs.Emsg("XrdOssGetStorageSystem", XrdNdnOfs.error_, -1,
                       "init XrdNdnOfs");
        return NULL;
    }

    XrdNdnOfs.eDest_->Say(
        "++++++ The Named Data Networking Storage System configuration...");

    XrdNdnOfs.eDest_->Say("       ofs NDNc consumer. gqlserver=",
                          XrdNdnOfs.options_.gqlserver.c_str());

    XrdNdnOfs.eDest_->Say("       ofs NDNc consumer. mtu=",
                          std::to_string(XrdNdnOfs.options_.mtu).c_str());

    XrdNdnOfs.eDest_->Say("       ofs NDNc consumer. prefix=",
                          XrdNdnOfs.options_.prefix.toUri().c_str());

    XrdNdnOfs.eDest_->Say(
        "       ofs NDNc consumer. interestLifetime=",
        std::to_string(XrdNdnOfs.options_.interestLifetime.count()).c_str());

    XrdNdnOfs.eDest_->Say(
        "       ofs NDNc consumer. pipelineType=",
        std::to_string(XrdNdnOfs.options_.pipelineType).c_str());

    XrdNdnOfs.eDest_->Say(
        "       ofs NDNc consumer. pipelineSize=",
        std::to_string(XrdNdnOfs.options_.pipelineSize).c_str());

    XrdNdnOfs.eDest_->Say(
        "------ Named Data Networking Storage System configuration completed.");

    signal(SIGINT, sigint_handler);

    return ((XrdOss *)&XrdNdnOfs);
}
}

namespace xrdndnofs {
XrdNdnOss::XrdNdnOss() : XrdOss(), options_{}, consumer_{nullptr} {
}

XrdNdnOss::~XrdNdnOss() {
    if (eDest_ != nullptr) {
        eDest_->Say("dtor: XrdNdnOss");
    }

    if (consumer_ != nullptr) {
        consumer_->stop();
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

XrdOssDF *XrdNdnOss::newDir(const char *) {
    if (!this->consumer_->isValid()) {
        Emsg("newDir", XrdNdnOfs.error_, -1, "invalid consumer");
        return nullptr;
    }

    return (XrdNdnOssDir *)new XrdNdnOssDir(this->consumer_);
}

XrdOssDF *XrdNdnOss::newFile(const char *) {
    if (!this->consumer_->isValid()) {
        Emsg("newFile", XrdNdnOfs.error_, -1, "invalid consumer");
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

int XrdNdnOss::Init(XrdSysLogger *lp, const char *) {
    eDest_ = &OssEroute;
    eDest_->logger(lp);

    eDest_->Say("Copyright © 2022 California Institute of Technology\n"
                "Author: Catalin Iordache <catalin.iordache@cern.ch>");
    eDest_->Say("Named Data Networking storage system v",
                XRDNDNOSS_VERSION_STRING, " initialization.");

    if (consumer_ == nullptr) {
        consumer_ = std::make_shared<ndnc::posix::Consumer>(options_);
    }

    if (consumer_ == nullptr || !consumer_->isValid()) {
        Emsg("Init", XrdNdnOfs.error_, -1, "invalid consumer");
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

bool XrdNdnOss::Config(const char *parms) {
    std::string sparms(parms);
    { // Remove comments from params line in config file
        std::size_t foundComment = sparms.find("#");
        if (foundComment != std::string::npos)
            sparms = sparms.substr(0, foundComment);
    }

    if (sparms.empty()) {
        return false;
    }

    std::istringstream iss(sparms);
    std::vector<std::string> vparms(std::istream_iterator<std::string>{iss},
                                    std::istream_iterator<std::string>());

    if (vparms.size() % 2 != 0) {
        Emsg("Config", XrdNdnOfs.error_, -1,
             "the list of parameters in config file is not even. The custom "
             "arguments will be ignored");
        return true;
    }

    auto getStringFromParams = [&](std::string key, string &ret) {
        for (auto it = vparms.begin(); it != vparms.end(); ++it) {
            if (key.compare(*it) != 0)
                continue;
            ++it;
            ret = *it;
            return true;
        }

        return false;
    };

    auto getIntFromParams = [&](std::string key, int &ret) {
        for (auto it = vparms.begin(); it != vparms.end(); ++it) {
            if (key.compare(*it) != 0)
                continue;
            try {
                ret = std::stoi(*++it);
                return true;
            } catch (const std::exception &e) {
                Emsg("Config", XrdNdnOfs.error_, -1, e.what());
                return false;
            }
        }
        return false;
    };

    {
        std::string gqlserver = "";
        if (getStringFromParams("gqlserver", gqlserver)) {
            if (gqlserver.empty()) {
                Emsg("Config", XrdNdnOfs.error_, -1, "invalid gqlserver value");
                return false;
            } else {
                options_.gqlserver = gqlserver;
            }
        }
    }

    {
        int mtu = 0;
        if (getIntFromParams("mtu", mtu)) {
            if (mtu < 64 || mtu > 9000) {
                Emsg("Config", XrdNdnOfs.error_, -1, "invalid mtu value");
                return false;
            } else {
                options_.mtu = mtu;
            }
        }
    }

    {
        std::string prefix = "";
        if (getStringFromParams("prefix", prefix)) {
            if (prefix.empty()) {
                Emsg("Config", XrdNdnOfs.error_, -1, "invalid prefix value");
                return false;
            } else {
                options_.prefix = prefix;
            }
        }
    }

    {
        int interestLifetime = 0;
        if (getIntFromParams("interestLifetime", interestLifetime)) {
            if (interestLifetime < 0) {
                Emsg("Config", XrdNdnOfs.error_, -1,
                     "invalid interestLifetime value. this argument will be "
                     "ignored");
            } else {
                options_.interestLifetime =
                    ndn::time::milliseconds{interestLifetime};
            }
        }
    }

    {
        std::string pipelineType = "";
        if (getStringFromParams("pipelineType", pipelineType)) {
            if (pipelineType.empty()) {
                Emsg("Config", XrdNdnOfs.error_, -1,
                     "invalid pipelineType value. this argument will be "
                     "ignored");
            } else {
                if (pipelineType.compare("aimd") == 0) {
                    options_.pipelineType = ndnc::PipelineType::aimd;
                } else {
                    options_.pipelineType = ndnc::PipelineType::fixed;
                }
            }
        }
    }

    {
        int pipelineSize = 0;
        if (getIntFromParams("pipelineSize", pipelineSize)) {
            if (pipelineSize < 0) {
                Emsg("Config", XrdNdnOfs.error_, -1,
                     "invalid pipelineSize value. this argument will be "
                     "ignored");
            } else {
                options_.pipelineSize = pipelineSize;
            }
        }
    }

    return true;
}
}; // namespace xrdndnofs

XrdVERSIONINFO(XrdOssGetStorageSystem, "xrdndnoss")
