Name:    xrdndn-producer
Version: 0.1.1
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
%{?systemd_requires}
BuildRequires: systemd

%description
This is an NDN producer who compliments the NDN based OSS xrootd plugin.

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
make xrdndn-producer VERBOSE=1

%install
cd %{SrcDir}/build
make xrdndn-producer install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/ndn
cp ../rpms/xrdndn-producer/nfd.xrootd.conf.sample $RPM_BUILD_ROOT%{_sysconfdir}/ndn/nfd.xrootd.conf.sample
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/rsyslog.d/
cp ../rpms/xrdndn-producer/xrdndn-producer-rsyslog.conf $RPM_BUILD_ROOT%{_sysconfdir}/rsyslog.d/xrdndn-producer-rsyslog.conf
mkdir -p $RPM_BUILD_ROOT%{_prefix}/lib/systemd/system
cp ../rpms/xrdndn-producer/xrdndn-producer.service $RPM_BUILD_ROOT%{_prefix}/lib/systemd/system/xrdndn-producer.service
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/
cp ../rpms/xrdndn-producer/xrdndn-producer-environment $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/xrdndn-producer

%post
ln -s -f %{_prefix}/lib/systemd/system/xrdndn-producer.service  %{_prefix}/lib/systemd/system/multi-user.target.wants/xrdndn-producer.service
%systemd_post xrdndn-producer.service

%preun
%systemd_preun xrdndn-producer.service

%postun
unlink %{_prefix}/lib/systemd/system/multi-user.target.wants/xrdndn-producer.service
%systemd_postun_with_restart rdndn-producer.service
systemctl restart rsyslog.service

%posttrans
systemctl restart rsyslog.service
systemctl start xrdndn-producer.service

%files
%{_bindir}/xrdndn-producer
%{_sysconfdir}/ndn/nfd.xrootd.conf.sample
%{_sysconfdir}/sysconfig/xrdndn-producer
%{_prefix}/lib/systemd/system/xrdndn-producer.service
%{_sysconfdir}/rsyslog.d/xrdndn-producer-rsyslog.conf

%changelog
* Thu Sep 20 2018 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.1
- TODO
* Mon Sep 3 2018 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.0
- Initial NDN producer for NDN based OSS plugin for xrootd packaging.