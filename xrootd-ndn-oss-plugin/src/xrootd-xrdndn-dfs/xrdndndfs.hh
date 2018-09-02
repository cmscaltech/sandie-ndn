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

#ifndef XRDNDNDFS_HH
#define XRDNDNDFS_HH

#include <XrdOss/XrdOss.hh>
#include <XrdOuc/XrdOucErrInfo.hh>
#include <XrdOuc/XrdOucStream.hh>
#include <XrdSfs/XrdSfsAio.hh>
#include <XrdSys/XrdSysError.hh>
#include <XrdSys/XrdSysLogger.hh>
#include <XrdSys/XrdSysPlugin.hh>

// Forward declarations.
class XrdSfsAio;
class XrdSysLogger;

/*****************************************************************************/
/*                      X r d H d f s D i r e c t o r y                      */
/*****************************************************************************/
class XrdNdnDfsDirectory : public XrdOssDF {
  public:
    XrdNdnDfsDirectory(const char *) {}
    ~XrdNdnDfsDirectory() {}

  public:
    int Opendir(const char *, XrdOucEnv &) { return -ENOTDIR; }
    int Readdir(char *, int) { return -ENOTDIR; }
    int StatRet(struct stat *) { return -ENOTSUP; }
    int Close(long long * = 0) { return 0; }
};

/*****************************************************************************/
/*                           X r d H d f s F i l e                           */
/*****************************************************************************/
class XrdNdnDfsFile : public XrdOssDF {
  public:
    XrdNdnDfsFile(const char *);
    ~XrdNdnDfsFile();

  public:
    // Supported file system calls

    int Fstat(struct stat *);
    int Open(const char *, int, mode_t, XrdOucEnv &);
    ssize_t Read(off_t, size_t);
    ssize_t Read(void *, off_t, size_t);
    int Read(XrdSfsAio *aoip);
    ssize_t ReadRaw(void *, off_t, size_t);
    int Close(long long *retsz = 0);

  private:
    XrdOucErrInfo error;
    std::string filePath;

  public:
    int Fchmod(mode_t mode) {
        (void)mode;
        return -EISDIR;
    }
    int Fsync() { return -EISDIR; }
    int Fsync(XrdSfsAio *aiop) {
        (void)aiop;
        return -EISDIR;
    }
    int Ftruncate(unsigned long long) { return -EISDIR; }
    int getFD() { return -1; }
    off_t getMmap(void **addr) {
        (void)addr;
        return 0;
    }
    int isCompressed(char *cxidp = 0) {
        (void)cxidp;
        return -EISDIR;
    }
    ssize_t Write(const void *, off_t, size_t) { return (ssize_t)-EISDIR; }
    int Write(XrdSfsAio *aiop) {
        (void)aiop;
        return (ssize_t)-EISDIR;
    }
};

/*****************************************************************************/
/*                           X r d H d f s S y s                             */
/*****************************************************************************/
class XrdNdnDfsSys : public XrdOss {
  public:
    XrdNdnDfsSys() : XrdOss() {}
    virtual ~XrdNdnDfsSys() {}

  public:
    static int Emsg(const char *, XrdOucErrInfo &, int, const char *x,
                    const char *y = "");
    void Say(const char *, const char *x = "", const char *y = "",
             const char *z = "");

  public:
    XrdOssDF *newDir(const char *tident) {
        return (XrdNdnDfsDirectory *)new XrdNdnDfsDirectory(tident);
    }

    XrdOssDF *newFile(const char *tident) {
        return (XrdNdnDfsFile *)new XrdNdnDfsFile(tident);
    }
    int Chmod(const char *, mode_t, XrdOucEnv *) { return -ENOTSUP; }
    int Create(const char *, const char *, mode_t, XrdOucEnv &, int) {
        return -ENOTSUP;
    }
    int Init(XrdSysLogger *, const char *);
    int Mkdir(const char *, mode_t, int, XrdOucEnv *) { return -ENOTSUP; }
    int Remdir(const char *, int, XrdOucEnv *) { return -ENOTSUP; }
    int Rename(const char *, const char *, XrdOucEnv *, XrdOucEnv *) {
        return -ENOTSUP;
    }
    int Stat(const char *, struct stat *, int, XrdOucEnv *) { return -ENOTSUP; }
    int Truncate(const char *, unsigned long long, XrdOucEnv *) {
        return -ENOTSUP;
    }
    int Unlink(const char *, int, XrdOucEnv *) { return -ENOTSUP; }

    XrdSysError *m_eDest;
};

#endif // XRDNDNDFS_HH