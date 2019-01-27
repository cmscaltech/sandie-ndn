Name:    xrdndn-producer
Version: 0.1.4
Release: 1%{?dist}
Summary: Named Data Networking (NDN) Producer for XRootD plugin
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
This is the NDN Producer who compliments the NDN based File System XRootD Server plugin.

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
* Sun Jan 27 2019 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.4
- Added command line options for the application.
- To see the available command line options and how to use them run "xrdndn-producer -h".
- Minor bugs have been solved.

* Wed Oct 24 2018 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.3
- Performance improvements.
- There are 32 threads processing request on the ndn::Face.
- In addition, a FileHandler object has been introduced, which is processing one file using two threads.

* Fri Sep 21 2018 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.2
- On close interest a task is scheduled to close the file in 180s.
- Install NDN producer as a systemd service in CentOS 7 / RHEL.
- Resolved "corrupted data error" on XRootD Server.
- By default logging level is INFO. The logs are saved in /var/log/xrdndn-producer.log .
- The logs can also be seen using journalctl: journalctl -l -u xrdndn-producer .
- To change logging level edit /etc/sysconfig/xrdndn-producer .

* Mon Sep 3 2018 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.0
- Initial NDN producer for NDN based OSS plugin for xrootd packaging.