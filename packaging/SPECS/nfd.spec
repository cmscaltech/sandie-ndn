Name:       nfd
Version:    0.6.6
Release:    24%{?dist}
Summary:    A Named Data Networking (NDN) forwarder
License:    GPLv3+
URL:        http://named-data.net/downloads/
Source0:    http://named-data.net/downloads/nfd-%{version}.tar.bz2

BuildRequires:  pkgconfig libpcap-devel
#websocketpp-devel
BuildRequires:  python2 boost-devel libndn-cxx-devel

%description
NFD is a Named Data Networking (NDN) forwarder

%global debug_package %{nil}

%prep
%setup -qn nfd-%{version}

%build
CXXFLAGS="%{optflags} -std=c++14" \
%{__python2} ./waf --prefix=%{_prefix} --bindir=%{_bindir} --libdir=%{_libdir} \
 --datadir=%{_datadir} --sysconfdir=%{_sysconfdir} configure --without-websocket

%{__python2} ./waf -v %{?_smp_mflags}

%install
./waf install --destdir=%{buildroot} --prefix=%{_prefix} \
 --bindir=%{_bindir} --datadir=%{_datadir}

%files
%dir %{_sysconfdir}/ndn
%dir %{_datarootdir}/ndn/
%{_bindir}/*
%{_datarootdir}/ndn/*
%config(noreplace) %{_sysconfdir}/ndn/nfd.conf.sample
%config(noreplace) %{_sysconfdir}/ndn/autoconfig.conf.sample
%{_mandir}

%changelog
* Tue Apr 30 2019 Catalin Iordache <catalin.iordache@cern.ch> - 0.6.6
- Changelog: https://github.com/named-data/NFD/releases/tag/NFD-0.6.6

* Fri Nov 6 2015 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.3.4-1
- Initial NFD packaging
