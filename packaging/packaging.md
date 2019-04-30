# Guidelines for installation of NDN stack on Centos 7.5

We consider the NDN stack to be composed of: **boost**, **ndn-cxx**, **nfd**, **ndn-tools** and **xrootd**. Bellow are a few guidelines for anyone how wants to manually install these softwares as well as creating RPMs for them.

By installing these softwares from sources, a lot of the steps which are needed to create the packages are also completed. Consider the next sections as steps in booth installing the software and preparing your build system.

## Prerequisites

On a minimal installation of Centos 7.5, there are a lot of dependencies to install in order to build boost and the others, some of which are present in the commands bellow. Feel free to add missing ones.

```bash
caltech@cms:~# yum install  yum-utils valgrind wget sqlite-devel openssl-devel.x86_64 libtranslit-icu.x86_64 \
                            bzip2-devel.x86_64 doxygen graphviz python-sphinx libpcap-devel.x86_64 psmisc \
                            dpkg-devel dpkg-dev chrpath.x86_64 dos2unix.x86_64 expat-devel.x86_64 \
                            libicu-devel.x86_64 python-devel.x86_64 autoconf213.noarch docbook-utils.noarch \
                            docbook-utils-pdf.noarch docbook-style-xsl.noarch texlive-latex.noarch

caltech@cms:~# curl "https://bootstrap.pypa.io/get-pip.py" -o "get-pip.py" && chmod a+x get-pip.py && ./get-pip.py
```

Bzip2 packets may not install the **libbz2.so** needed for boost compilation. To manually compile and install it:

```bash
caltech@cms:~# mkdir libbz2_TEMP && cd libbz2_TEMP
caltech@cms:~# yumdownloader --source bzip2-devel
caltech@cms:~# rpm2cpio bzip2-1.0.6-13.el7.src.rpm |cpio -idv
caltech@cms:~# tar xf bzip2-1.0.6.tar.gz
caltech@cms:~# patch -p0 < bzip2-1.0.6-saneso.patch
caltech@cms:~# patch -p0 < bzip2-1.0.6-cflags.patch
caltech@cms:~# cd bzip2-1.0.6
caltech@cms:~# patch -p1 < ../bzip2-1.0.6-bzip2recover.patch
caltech@cms:~# make -f Makefile-libbz2_so
caltech@cms:~# cp libbz2.so.1.0.6 /usr/local/lib/
caltech@cms:~# ls -s /usr/local/lib/libbz2.so.1.0.6 /usr/local/lib/libbz2.so
```

## gcc

For **ndn-cxx 0.6.6** at least gcc 5.3.0 is required. For our purpose, we are using **gcc 8.2.1**. You can install it from [Centos Software Collections (SCL) Repository](https://wiki.centos.org/AdditionalResources/Repositories/SCL) with the following commands:

```bash
caltech@cms:~# yum install centos-release-scl
caltech@cms:~# yum install devtoolset-8-gcc*
caltech@cms:~# scl enable devtoolset-8 bash
```

## boost

With the commands bellow, you clone the boost git repository: [Boostorg/boost](https://github.com/boostorg/boost), build the libraries and install them.

```bash
caltech@cms:~# git clone https://github.com/boostorg/boost.git
caltech@cms:~# git submodule update --init
caltech@cms:~# cd boost
caltech@cms:~# ./bootstrap.sh
caltech@cms:~# ./b2 -j 4
caltech@cms:~# ./b2 install--with=all
caltech@cms:~# export LD_LIBRARY_PATH=/usr/local/lib/:$LD_LIBRARY_PATH
caltech@cms:~# cat /usr/include/boost/version.hpp
```

## ndn-cxx

With the commands bellow, you clone the [ndn-cxx](https://github.com/named-data/ndn-cxx) git repository, compile the source files and install the libraries.

```bash
caltech@cms:~# git clone https://github.com/named-data/ndn-cxx.git
caltech@cms:~# cd ndn-cxx
caltech@cms:~# git submodule update --init
caltech@cms:~# export LC_ALL=en_US.UTF-8
caltech@cms:~# export LANG=en_US.UTF-8
caltech@cms:~# ./waf configure --with-examples
caltech@cms:~# ./waf
caltech@cms:~# ./waf install
```

## NFD

With the commands bellow, you clone the [NFD](https://github.com/named-data/NFD) git repository, compile the source files and install the libraries.

```bash
caltech@cms:~# git clone https://github.com/named-data/NFD.git
caltech@cms:~# cd NFD
caltech@cms:~# git submodule update --init
caltech@cms:~# export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig/:$PKG_CONFIG_PATH
caltech@cms:~# ./waf configure --with-tests
caltech@cms:~# ./waf
caltech@cms:~# ./waf install
caltech@cms:~# cp nfd.conf.sample /usr/local/etc/ndn/nfd.conf
```

## NDN Essential Tools

With the following commands, you clone the [NDN Essential Tools](https://github.com/named-data/ndn-tools.git) git repository, compile the source files and install the binaries:

```bash
caltech@cms:~# git clone https://github.com/named-data/ndn-tools.git
caltech@cms:~# cd ndn-tools
caltech@cms:~# git submodule update --init
caltech@cms:~# ./waf configure --with-tests
caltech@cms:~# ./waf
caltech@cms:~# ./waf install
```

## Packaging - How to create an RPM

In order to create an RPM from your sources you need to install the **rpmdevtools**. You can do this with the following command:
```bash
caltech@cms:~# yum install rpmdevtools
```

Once the **rpmbuild** application is installed on your machine you need to setup a directory tree:
```bash
caltech@cms:~# rpmdev-setuptree
```
The above command creates *rpmbuild* directory in the path were the command was runned. Inside this, you will find more directories. The one of interest are:
- *SOURCES*: where you need to pun your source files
- *SPECS*: where you put the spec file
- *RPMS*: where the resulted rpm will be put
- *SRPMS*: where the source code along with the spec file will be archived

After you've copied your source and spec files in the required locations, you need to go back in the parent directory of *rpmbuild* and run the following command:
```bash
caltech@cms:~# spectool -g -R rpmbuild/SPECS/<spec-file-name>.spec
caltech@cms:~# rpmbuild -v -ba rpmbuild/SPECS/<spec-file-name>.spec
```

In the end you can see the content of the rpm file:
```bash
caltech@cms:~# rpm -qpl rpmbuild/RPMS/<arch>/<rpm-name>.rpm
```

In this repository you can find both rpms and specs for: [boost 1.58.0](SPECS/boost-1.58.0.spec), [boost 1.69.0](SPECS/boost-1.69.0.spec), [ndn-cxx 0.6.6](SPECS/libndn-cxx.spec), [NFD 0.6.6](SPECS/nfd.spec) and [ndn-tools 0.6.4](SPECS/ndn-tools.spec).

## xrootd

To install XRootD on Centos 7.5 follow the [link](https://opensciencegrid.org/docs/data/xrootd/install-standalone/) on opensciencegrid website.
