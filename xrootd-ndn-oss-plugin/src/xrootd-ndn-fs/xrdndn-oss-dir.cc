// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
// 
// Author: Catalin Iordache <catalin.iordache@cern.ch>
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "xrdndn-oss-dir.hh"

XrdNdnOssDir::XrdNdnOssDir(const char *) {}

XrdNdnOssDir::~XrdNdnOssDir() {}

int XrdNdnOssDir::Opendir(const char *, XrdOucEnv &) { return -ENOTDIR; }

int XrdNdnOssDir::Readdir(char *, int) { return -ENOTDIR; }

int XrdNdnOssDir::StatRet(struct stat *) { return -ENOTSUP; }

int XrdNdnOssDir::Close(long long *) { return 0; }
