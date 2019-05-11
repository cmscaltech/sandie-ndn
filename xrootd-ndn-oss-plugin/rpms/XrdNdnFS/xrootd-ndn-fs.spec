Name:    xrootd-ndn-fs
Version: 0.2.0
Release: 4%{?dist}
Summary: Named Data Networking (NDN) based Open Storage System plugin for XRootD
Group:   System Environment/Development
License: GPLv3+
URL:     https://github.com/cmscaltech/sandie-ndn.git

#BuildRequires: cmake
BuildRequires: boost-devel
BuildRequires: gcc-c++
BuildRequires: libndn-cxx-devel >= 0.6.6
BuildRequires: xrootd-devel >= 4.8.0
BuildRequires: xrootd-server-devel >= 4.8.0
AutoReqProv: no

%description
The Named Data Networking (NDN) based Open File System XRootD plugin component
for Open Storage System. Supported file system calls: open, fstat, read and close.

%define RepoName sandie-ndn
%define SrcDir %{RepoName}/xrootd-ndn-oss-plugin

%prep
rm -rf %{RepoName}
git clone %{url}
cd %{SrcDir}
git checkout tags/%{version}

%build
cd %{SrcDir}
mkdir build
cd build
cmake ../
make XrdNdnFS -j2 VERBOSE=1

%install
cd %{SrcDir}/build
make XrdNdnFS install DESTDIR=$RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/xrootd
cp ../rpms/XrdNdnFS/xrootd-ndn.sample.cfg $RPM_BUILD_ROOT%{_sysconfdir}/xrootd/xrootd-ndn.sample.cfg
cp ../rpms/XrdNdnFS/xrootd-ndn.sample.cfg $RPM_BUILD_ROOT%{_sysconfdir}/xrootd/xrootd-ndn.cfg

%preun
%systemd_preun xrootd@ndn.service

%files
%{_libdir}/libXrdNdnFS.so
%{_sysconfdir}/xrootd/xrootd-ndn.sample.cfg
%config(noreplace) %{_sysconfdir}/xrootd/xrootd-ndn.cfg

%changelog
* Sat May 11 2019 Catalin Iordache <catalin.iordache@cern.ch> - 0.2.0
- Support for multi-threading XRootD NDN Consumer. One Consumer instance per file
- Added interest-lifetime option: Configure Interest lifetime
- Added pipeline-size option: Control the number of Interest expressed in parallel in fixed window size pipeline
- Added log-level option: No neeed to export any environment variables to set log level
- Options can are configurable from /etc/xrootd/xrootd-ndn.cfg file as ofs.osslib arguments
- Consumer options are configurable from XRootD using xrdndnconsumer::Options structure
- Removed close Interest
- Added support for adequate error codes for XRootD
- Added suport for ndn-cxx 0.6.6 API
- Bug fixing

* Sun Jan 27 2019 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.4
- Minor bugs have been solved.

* Wed Oct 24 2018 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.3
- More verbose on the NACK and Timeout scenarios.
- Small bug fixes.

* Fri Sep 21 2018 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.2
- Handle Consumer runtime errors gracefully.
- Close return value from Producer is not be considered as NDN is stateless.

* Mon Sep 3 2018 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.0
- Initial NDN based OSS plugin for xrootd packaging.