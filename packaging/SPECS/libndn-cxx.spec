Name:       libndn-cxx
Version:    0.6.6
Release:    24%{?dist}
Summary:    C++ library implementing Named Data Networking primitives
License:    LGPLv3+ and (BSD or LGPLv3+) and (GPLv3+ or LGPLv3+)
URL:        http://named-data.net
Source0:    http://named-data.net/downloads/ndn-cxx-%{version}.tar.bz2

#boost-devel cryptopp-devel
BuildRequires:  sqlite-devel pkgconfig openssl-devel
#boost-python-devel
BuildRequires:  python2-devel python2-devel

%description
libndn-cxx is a C++ library that implements NDN primitives.

%package  devel
Summary:  Development files for %{name}
Requires: %{name}%{?_isa} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%prep
%setup -qn ndn-cxx-%{version}

%build
CXXFLAGS="%{optflags} -std=c++14" \
LINKFLAGS="-Wl,--as-needed" \
%{__python2} ./waf --enable-shared --disable-static \
--prefix=%{_prefix} --bindir=%{_bindir} --libdir=%{_libdir} \
--datadir=%{_datadir} --sysconfdir=%{_sysconfdir} configure

%{__python2} ./waf -v %{?_smp_mflags}

%install
./waf install --destdir=%{buildroot} --prefix=%{_prefix} \
 --bindir=%{_bindir} --libdir=%{_libdir}

%check
export LD_LIBRARY_PATH=$PWD/build

%post   -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%{_libdir}/libndn-cxx.so.%{version}
%doc AUTHORS.md  README-dev.md  README.md
%dir %{_sysconfdir}/ndn
%license COPYING.md
%{_bindir}/*
%config(noreplace) %{_sysconfdir}/ndn/client.conf.sample

%files devel
%{_includedir}/ndn-cxx/
%{_libdir}/libndn-cxx.so
%{_libdir}/pkgconfig/libndn-cxx.pc
%{_mandir}

%changelog
* Tue Apr 30 2019 Catalin Iordache <catalin.iordache@cern.ch> - 0.6.6
- Changelog: https://github.com/named-data/ndn-cxx/releases/tag/ndn-cxx-0.6.6

* Thu Mar 15 2018 Iryna Shcherbina <ishcherb@redhat.com> - 0.6.1-3
- Update Python 2 dependency declarations to new packaging standards
  (See https://fedoraproject.org/wiki/FinalizingFedoraSwitchtoPython3)

* Fri Feb 23 2018 Rex Dieter <rdieter@fedoraproject.org> - 0.6.1-2
- rebuild (cryptopp)

* Mon Feb 19 2018 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.6.1-1
- Package for 0.6.1 release

* Wed Feb 07 2018 Fedora Release Engineering <releng@fedoraproject.org> - 0.6.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_28_Mass_Rebuild

* Tue Jan 30 2018 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.6.0-1
- Package for 0.6.0 release

* Tue Apr 05 2016 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.4.1-3
- Package for 0.4.1 release

* Sun Jan 24 2016 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.4.0-2
- Rebuild for boost missing dependency

* Fri Jan 8 2016 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.4.0-1
- Package for 0.4.0 release

* Thu Nov 26 2015 Marcin Juszkiewicz <mjuszkiewicz@redhat.com> - 0.3.4-6
- Fix build on AArch64 with upstream fix

* Tue Nov 10 2015 Marcin Juszkiewicz <mjuszkiewicz@redhat.com> - 0.3.4-5
- Fix FTBFS on AArch64. Developer assumed that memcmp() returns -1/0/1 instead
  of reading what C/C++ standard says about it.

* Thu Oct 15 2015 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.3.4-4
- Fix unused-direct-shlib-dependency warning
- Add missing sysconfigdir

* Thu Oct 1 2015 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.3.4-3
- Add patch for boost 1.59 semicolon bug

* Thu Sep 24 2015 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.3.4-2
- Fix licencing
- Address reviewer's comments

* Wed Sep 16 2015 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.3.4-1
- Repackage for 0.3.4

* Tue Sep 08 2015 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.3.3-2
- Use waf to install files
- Fix review comments

* Fri Aug 21 2015 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.3.3-1
- Update package for 0.3.3

* Thu Feb 5 2015 Susmit Shannigrahi <susmit at cs.colostate.edu> - 0.3.0-1
- Initial Packaging

