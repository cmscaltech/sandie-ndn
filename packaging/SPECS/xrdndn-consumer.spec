Name:    xrdndn-consumer
Version: 0.2.0
Release: 4%{?dist}
Summary: Named Data Networking (NDN) Consumer used in the NDN Open Storage System plugin for XRootD
Group:   System Environment/Development
License: GPLv3+
URL:     https://github.com/cmscaltech/sandie-ndn.git

#BuildRequires: cmake
BuildRequires: boost-devel
BuildRequires: gcc-c++
BuildRequires: libndn-cxx-devel >= 0.6.6
AutoReqProv: no

%description
This is the NDN Consumer used in the Named Data Networking (NDN) based File
System XRootD plugin component for Open Storage System (OSS). It is delivered as a
standalone application for testing the NDN Producer (xrdndn-producer service)
which compliments the NDN OSS XRootD Server plugin.

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

%files
%{_bindir}/%{name}

%changelog
* Sat May 11 2019 Catalin Iordache <catalin.iordache@cern.ch> - 0.2.0
- Support for multi-threading XRootD NDN Consumer. One Consumer instance per file
- Added interest-lifetime command line option: Configure Interest lifetime
- Added nthreads command line option: Configure the number of threads to read concurrently from file over NDN
- Added pipeline-size command line option: Control the number of Interest expressed in parallel in fixed window size pipeline
- Added log-level command line option: Log level will be set via command line, no neeed to export any environment variables
- Added V command line argument to show the version number
- Other command line arguments: read buffer size (bsize), input file (input-file), output-file (output file)
- Removed close Interest
- Added support for adequate error codes for XRootD
- Added suport for ndn-cxx 0.6.6 API
- Bug fixing

* Sun Jan 27 2019 Catalin Iordache <catalin.iordache@cern.ch> - 0.1.4
- Initial NDN Consumer also used in the Named Data Networking (NDN) based Open Storage System plugin for XRootD.
- This application is intended to be used for tests only.