Name:    xrootd-ndn-fs
Version: 0.1.0
Release: 1%{?dist}
Summary: Named Data Networking (NDN) based Open Storage System plugin for xrootd
Group:   System Environment/Development
License: GPLv3+
URL:     https://github.com/cmscaltech/sandie-ndn.git

BuildRequires: cmake
BuildRequires: boost-devel
BuildRequires: gcc-c++
BuildRequires: libndn-cxx-devel >= 0.6.2
BuildRequires: xrootd-devel >= 4.8.0
BuildRequires: xrootd-server-devel >= 4.8.0

%description
This is a plugin for xrootd. It implements an Named Data Networking (NDN) based
OSS. Currently it offers support for: open, fstat, close and read file system
calls.

%define RepoName sandie-ndn
%define SrcDir %{RepoName}/xrootd-ndn-oss-plugin

%prep
rm -rf %{RepoName}
git clone %{url}

%build
cd %{SrcDir}
mkdir build
cd build
cmake ../
make XrdNdnFS VERBOSE=1
make XrdNdnFSReal VERBOSE=1

%install
cd %{SrcDir}/build
make XrdNdnFS install DESTDIR=$RPM_BUILD_ROOT
make XrdNdnFSReal install DESTDIR=$RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/xrootd
cp ../rpm/xrootd.sample.ndnfd.cfg $RPM_BUILD_ROOT%{_sysconfdir}/xrootd/xrootd.sample.ndnfd.cfg

%files
%{_libdir}/libXrdNdnFS.so
%{_libdir}/libXrdNdnFSReal.so
%{_sysconfdir}/xrootd/xrootd.sample.ndnfd.cfg

%changelog
* Mon Sep 3 2018 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.0
- Initial NDN based OSS plugin for xrootd packaging.