Name:    xrdndn-producer
Version: 0.1.0
Release: 1%{?dist}
Summary: Named Data Networking (NDN) Producer for xrootd plugin
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
This is an NDN producer who compliments the NDN based OSS xrootd plugin.

%define RepoName sandie-ndn
%define SrcDir %{RepoName}/xrootd-ndn-oss-plugin

%prep
# rm -rf %{RepoName}
# git clone %{url}

%build
cd %{SrcDir}
mkdir build
cd build
cmake ../
make xrdndn-producer VERBOSE=1

%install
cd %{SrcDir}/build
make xrdndn-producer install DESTDIR=$RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/ndn
cp ../rpms/nfd.xrootd.conf.sample $RPM_BUILD_ROOT%{_sysconfdir}/ndn/nfd.xrootd.conf.sample

%files
%{_bindir}/xrdndn-producer
%{_sysconfdir}/ndn/nfd.xrootd.conf.sample

%changelog
* Mon Sep 3 2018 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.0
- Initial NDN producer for NDN based OSS plugin for xrootd packaging.