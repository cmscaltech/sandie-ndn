Name:       nfd
Version:    0.6.2
Release:    1%{?dist}
Summary:    A Named Data Networking (NDN) forwarder
License:    GPLv3+
URL:        http://named-data.net/downloads/
Source0:    http://named-data.net/downloads/nfd-%{version}.tar.bz2
#boost-devel websocketpp-devel libndn-cxx-devel
BuildRequires:  pkgconfig libpcap-devel
BuildRequires:  python2

%description
NFD is a Named Data Networking (NDN) forwarder

%global debug_package %{nil}

%prep
%setup -qn nfd-%{version}

%build
CXXFLAGS="%{optflags} -std=c++11" \
%{__python2} ./waf --with-tests --prefix=%{_prefix} --bindir=%{_bindir} --libdir=%{_libdir} \
 --datadir=%{_datadir} --sysconfdir=%{_sysconfdir} configure --without-websocket

%{__python2} ./waf -v %{?_smp_mflags}

%install
./waf install --destdir=%{buildroot} --prefix=%{_prefix} \
 --bindir=%{_bindir} --datadir=%{_datadir}

%check
#build/unit-tests-core
#build/unit-tests-daemon
#build/unit-tests-rib

%files
%dir %{_sysconfdir}/ndn
%dir %{_datarootdir}/ndn/
%{_bindir}/*
%{_datarootdir}/ndn/*
%config(noreplace) %{_sysconfdir}/ndn/nfd.conf.sample
%config(noreplace) %{_sysconfdir}/ndn/autoconfig.conf.sample
%{_mandir}

%changelog
* Fri Nov 6 2015 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.3.4-1
- Initial NFD packaging
