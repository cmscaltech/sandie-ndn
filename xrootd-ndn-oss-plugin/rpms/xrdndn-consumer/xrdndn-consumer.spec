Name:    xrdndn-consumer
Version: 0.1.4
Release: 1%{?dist}
Summary: Named Data Networking (NDN) Consumer used in the NDN based Open Storage System plugin for XRootD 
Group:   System Environment/Development
License: GPLv3+
URL:     https://github.com/cmscaltech/sandie-ndn.git

BuildRequires: cmake
BuildRequires: boost-devel
BuildRequires: gcc-c++
BuildRequires: libndn-cxx-devel >= 0.6.2

%description
This is the NDN Consumer used in the Named Data Networking (NDN) based File System
XRootD plugin. It is delivered as a standalone application for testing purposes of the
NDN Producer who compliments the NDN based File System XRootD Server plugin (xrdndn-producer).

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
make xrdndn-consumer VERBOSE=1

%install
cd %{SrcDir}/build
make xrdndn-consumer install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/
cp ../rpms/xrdndn-consumer/xrdndn-consumer-environment $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/xrdndn-consumer

%files
%{_bindir}/xrdndn-consumer
%{_sysconfdir}/sysconfig/xrdndn-consumer

%changelog
* Sun Jan 27 2019 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.4
- Initial NDN Consumer also used in the Named Data Networking (NDN) based Open Storage System plugin for XRootD.
- This application is intended to be used for tests only.