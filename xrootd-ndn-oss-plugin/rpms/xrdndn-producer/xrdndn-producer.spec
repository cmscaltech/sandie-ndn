Name:    xrdndn-producer
Version: 0.2.0
Release: 4%{?dist}
Summary: Named Data Networking (NDN) Producer service for NDN Open Storage System plugin for XRootD
Group:   System Environment/Development
License: GPLv3+
URL:     https://github.com/cmscaltech/sandie-ndn.git

#BuildRequires: cmake
BuildRequires: boost-devel
BuildRequires: gcc-c++
BuildRequires: libndn-cxx-devel >= 0.6.6
BuildRequires: xrootd-devel >= 4.8.5
BuildRequires: xrootd-server-devel >= 4.8.5
%{?systemd_requires}
BuildRequires: systemd
AutoReqProv: no

%description
This is the Named Data Networking (NDN) Producer service part of the NDN based
File System XRootD plugin component for Open Storage System. This Producer is
able to respond with Data to Interest packets expressed by the XRootD plugin.

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
make %{name} -j2 VERBOSE=1

%install
cd %{SrcDir}/build
make %{name} install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/ndn
cp ../rpms/%{name}/nfd.%{name}.conf.sample $RPM_BUILD_ROOT%{_sysconfdir}/ndn/nfd.%{name}.conf.sample
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/rsyslog.d/
cp ../rpms/%{name}/%{name}-rsyslog.conf $RPM_BUILD_ROOT%{_sysconfdir}/rsyslog.d/%{name}-rsyslog.conf
mkdir -p $RPM_BUILD_ROOT%{_prefix}/lib/systemd/system
cp ../rpms/%{name}/%{name}.service $RPM_BUILD_ROOT%{_prefix}/lib/systemd/system/%{name}.service
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/%{name}/
cp ../rpms/%{name}/%{name}.sample.cfg $RPM_BUILD_ROOT%{_sysconfdir}/%{name}/%{name}.cfg
cp ../rpms/%{name}/%{name}.sample.cfg $RPM_BUILD_ROOT%{_sysconfdir}/%{name}/%{name}.sample.cfg

%post
ln -s -f %{_prefix}/lib/systemd/system/%{name}.service  %{_prefix}/lib/systemd/system/multi-user.target.wants/%{name}.service
%systemd_post %{name}.service

%preun
%systemd_preun %{name}.service

%postun
unlink %{_prefix}/lib/systemd/system/multi-user.target.wants/%{name}.service
%systemd_postun_with_restart %{name}.service
systemctl restart rsyslog.service
rm -rf %{_localstatedir}/log/%{name}

%posttrans
systemctl restart rsyslog.service
systemctl start %{name}.service

%files
%dir %{_sysconfdir}/ndn
%dir %{_sysconfdir}/%{name}
%{_bindir}/%{name}
%{_prefix}/lib/systemd/system/%{name}.service
%{_sysconfdir}/ndn/nfd.%{name}.conf.sample
%{_sysconfdir}/rsyslog.d/%{name}-rsyslog.conf
%{_sysconfdir}/%{name}/%{name}.sample.cfg
%config(noreplace) %{_sysconfdir}/%{name}/%{name}.cfg

%changelog
* Sat May 11 2019 Catalin Iordache <catalin.iordache@cern.ch> - 0.2.0
- Added support for async file read using async thread pool to increase throughput
- Remove close Interest filter. Implemented Garbage Collector to take care of closing files
- Added disable-signing command line option: To eliminate signing among authorized parteners
- Added freshness-period command line option: To set Interest freshness time
- Added garbage-collector-timer: To control how often Garbage collector will check to close files
- Added garbage-collector-lifetime: To set how much time a file will be left open before closing it
- Added log-level command line option: Log level will be set via command line
- Added nthreads command line option: To control how many threads will process Interests in parallel
- Added V command line argument to show the version number
- By using xrdndn-producer as a systemd service, all command line parameters can be set via /etc/xrdndn-producer/xrdndn-producer.cfg config file
- Added support for adequate error codes for XRootD
- Added suport for ndn-cxx 0.6.6 API
- Bug fixing

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