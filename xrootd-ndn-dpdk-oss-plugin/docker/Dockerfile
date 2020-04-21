FROM roboxes/centos8:2.0.6
ARG USER=sandie
ARG UID=1000
ARG GID=1000
ARG PW=sandie

RUN yum install -y sudo
RUN echo "${USER} ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# Using unencrypted password/ specifying password
RUN useradd --create-home --shell /bin/bash -m ${USER} --uid=${UID}
RUN set -o pipefail && echo "${USER}:${PW}" | chpasswd

# Setup default user at login
USER ${UID}:${GID}
ENV HOME /home/${USER}
WORKDIR ${HOME}

# Install available prerequisites
RUN sudo yum -y install epel-release
RUN sudo yum -y update
RUN sudo yum config-manager --set-enabled PowerTools
RUN sudo yum -y install CUnit-devel bind-utils vim htop git wget tar rpm-build rake doxygen \
        cmake cmake3 make tcsh lsof tk tcl autoconf automake perl perl-GD elfutils-libelf-devel \
        openssl-devel yamllint libtool socat glibc-devel.i686 glibc-devel.x86_64 gcc-gfortran \
        kernel-rpm-macros libfdt numad numactl-devel numactl-libs libpcap-devel \
        libbsd-devel jansson-devel nvme-cli createrepo libaio-devel bzip2-devel libffi-devel \
        pciutils pandoc meson ninja-build libnl3-devel libudev-devel pkgconfig libuuid-devel nasm \
        kernel-devel-4.18.0-147.5.1.el8_1.x86_64 kernel-headers-4.18.0-147.5.1.el8_1.x86_64

RUN sudo rpm -i https://netix.dl.sourceforge.net/project/ltp/Coverage%20Analysis/LCOV-1.14/lcov-1.14-1.noarch.rpm

# Install Node.js 12.x
RUN curl -sL https://rpm.nodesource.com/setup_12.x | sudo bash -
RUN sudo yum install -y nodejs

# Install Python 3.8
RUN set -o pipefail && curl -L https://www.python.org/ftp/python/3.8.2/Python-3.8.2.tgz | tar -C . -xz
WORKDIR ${HOME}/Python-3.8.2
RUN ./configure --enable-optimizations && sudo make -j32 altinstall
RUN alias python3=python3.8
WORKDIR ${HOME}
RUN sudo rm -rf Python-3.8.2

# Install jsonrpc2client 1.1.3
RUN set -o pipefail && curl -sfL https://github.com/powerman/rpc-codec/releases/download/v1.1.3/jsonrpc2client-linux-x86_64 | \
        sudo install /dev/stdin /usr/local/bin/jsonrpc2client

# Install Golang 1.14
RUN set -o pipefail && curl -L https://dl.google.com/go/go1.14.linux-amd64.tar.gz | sudo tar -C /usr/local -xz
RUN mkdir -p ${HOME}/go/src/
RUN sudo chown -R ${USER}:${USER} ${HOME}/go

# Configure environments
RUN sudo sh -c "printf ' \
# .bash_profile\n \
\n \
# Get the aliases and functions\n \
if [ -f ~/.bashrc ]; then\n \
        . ~/.bashrc\n \
fi\n \
\n \
# User specific environment and startup programs\n \
\n \
export GOROOT=/usr/local/go\n \
export GOPATH=\${HOME}/go\n \
export PATH=$PATH:$GOROOT/bin:$GOPATH/bin:/usr/local/lib/:/usr/local/bin/\n \
'  > ${HOME}/.bash_profile"

ENV GOROOT=/usr/local/go
ENV GOPATH=${HOME}/go
ENV PATH=/usr/local/go/bin/:${HOME}/go/bin/:/usr/local/lib/:/usr/local/bin/:/usr/local/sbin:/usr/sbin:/usr/bin:/sbin:/bin

# Install go-bindata
RUN go get github.com/jteeuwen/go-bindata
WORKDIR $GOPATH/src/github.com/jteeuwen/go-bindata/go-bindata
RUN go build && sudo mv go-bindata $GOROOT/bin/
WORKDIR ${HOME}

# Install llvm 6.0
RUN set -o pipefail && curl -L http://releases.llvm.org/6.0.0/llvm-6.0.0.src.tar.xz | tar -C . -xJ
RUN set -o pipefail && curl -L http://releases.llvm.org/6.0.0/cfe-6.0.0.src.tar.xz | tar -C llvm-6.0.0.src/tools/ -xJ
RUN mkdir build-clang
WORKDIR ${HOME}/build-clang
RUN cmake -DCMAKE_CXX_FLAGS:STRING="-Wno-unused-but-set-variable -Wno-unused-function -Wno-class-memaccess -Wno-cast-function-type -Wno-implicit-fallthrough -Wno-stringop-truncation -Wno-ignored-qualifiers" \
        -DLLVM_TARGETS_TO_BUILD="X86;BPF" -DLLVM_TARGET_ARCH="X86_64" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=on ../llvm-6.0.0.src
RUN sudo make install -j32
WORKDIR ${HOME}
RUN sudo rm -rf llvm-6.0.0.src build-clang

# Install Intel Multi-Buffer Crypto for IPSec v0.53
RUN git clone https://github.com/intel/intel-ipsec-mb.git
WORKDIR ${HOME}/intel-ipsec-mb
RUN git checkout tags/v0.53
RUN make -j16 && sudo make install
WORKDIR ${HOME}
RUN rm -rf intel-ipsec-mb

# Install liburcu
RUN set -o pipefail && curl -L https://github.com/urcu/userspace-rcu/archive/v0.11.1.tar.gz | tar -C . -xz
WORKDIR ${HOME}/userspace-rcu-0.11.1
RUN ./bootstrap && ./configure && make && sudo make install
RUN sudo sh -c "printf '/usr/local/lib' > /etc/ld.so.conf.d/liburcu.conf"
RUN sudo ldconfig
WORKDIR ${HOME}
RUN rm -rf userspace-rcu-0.11.1

# Install ubpf
RUN git clone https://github.com/iovisor/ubpf.git
WORKDIR ${HOME}/ubpf
RUN make -C vm && cd vm && sudo make install
WORKDIR ${HOME}
RUN rm -rf ubpf

# Install MLNX OFED drivers
RUN set -o pipefail && curl -L http://content.mellanox.com/ofed/MLNX_OFED-5.0-2.1.8.0/MLNX_OFED_LINUX-5.0-2.1.8.0-rhel8.1-x86_64.tgz | tar -C . -xz
WORKDIR ${HOME}/MLNX_OFED_LINUX-5.0-2.1.8.0-rhel8.1-x86_64
RUN set -o pipefail && sudo ./mlnxofedinstall -vvv --force --all --without-hpc --without-guest --without-hypervisor --without-fabric-collector --without-libdisni --add-kernel-support || true
WORKDIR ${HOME}
RUN rm -rf MLNX_OFED_LINUX-5.0-2.1.8.0-rhel8.1-x86_64

# Install MLNX toolset
RUN set -o pipefail && curl -L https://www.mellanox.com/downloads/MFT/mft-4.14.0-105-x86_64-rpm.tgz | tar -C . -xz
WORKDIR ${HOME}/mft-4.14.0-105-x86_64-rpm
RUN sudo ./install.sh
WORKDIR ${HOME}
RUN rm -rf mft-4.14.0-105-x86_64-rpm

# Install rdma core
RUN set -o pipefail && curl -L https://github.com/linux-rdma/rdma-core/releases/download/v25.0/rdma-core-25.0.tar.gz | tar -C . -xz
WORKDIR ${HOME}/rdma-core-25.0
RUN ./build.sh
RUN mkdir ninja_build
WORKDIR ${HOME}/rdma-core-25.0/ninja_build
RUN CFLAGS=-fPIC cmake -DIN_PLACE=1 -DENABLE_STATIC=1 -GNinja ..
RUN sudo ninja
WORKDIR ${HOME}
RUN rm -rf rdma-core-25.0

# Install DPDK v19.11 with ndn-dpdk patches
RUN git clone https://github.com/DPDK/dpdk.git
WORKDIR ${HOME}/dpdk
RUN git checkout tags/v19.11
RUN curl -L https://patches.dpdk.org/patch/65156/raw/ >> 65156.patch && git apply 65156.patch && rm -f 65156.patch
RUN curl -L https://patches.dpdk.org/patch/65158/raw/ >> 65158.patch && git apply 65158.patch && rm -f 65158.patch
RUN curl -L https://patches.dpdk.org/patch/65270/raw/ >> 65270.patch && git apply 65270.patch && rm -f 65270.patch
RUN sed -ri 's,(CONFIG_RTE_BUILD_SHARED_LIB=).*,\1y,' ./config/common_base && \
        sed -ri 's,(CONFIG_RTE_LIBRTE_BPF_ELF=).*,\1y,' ./config/common_base && \
        sed -ri 's,(CONFIG_RTE_LIBRTE_PMD_OPENSSL=).*,\1y,' ./config/common_base && \
        sed -ri 's,(CONFIG_RTE_LIBRTE_MLX5_PMD=).*,\1y,' ./config/common_base && \
        sed -ri 's,(RTE_MACHINE=).*,\1y,' ./config/common_base && \
        sed -ri 's,(CONFIG_RTE_BUILD_SHARED_LIB=).*,\1y,' ./config/common_base && \
        sed -ri 's,(CONFIG_RTE_IBVERBS_LINK_DLOPEN=).*,\1y,' ./config/common_base
RUN CC=gcc meson -Dtests=false --libdir=lib build
WORKDIR ${HOME}/dpdk/build
RUN sudo ninja install
RUN sudo ldconfig
WORKDIR ${HOME}
RUN sudo rm -rf dpdk

RUN sudo sh -c "printf '\nexport PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig\n' &>> ${HOME}/.bash_profile"
ENV PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig

# Install SPDK v19.10.1
RUN git clone https://github.com/spdk/spdk.git
WORKDIR ${HOME}/spdk
RUN git checkout tags/v19.10.1
RUN sed -ri '/SPDK_MEMPOOL_DEFAULT_CACHE_SIZE/ c\#define SPDK_MEMPOOL_DEFAULT_CACHE_SIZE 512' include/spdk/env.h
RUN CC=gcc LDFLAGS="-lnuma -lbsd" ./configure --enable-debug --disable-tests --with-shared --with-dpdk=/usr/local --without-vhost --without-isal --without-fuse
RUN make -j32
RUN sudo make install
RUN sudo ldconfig
WORKDIR ${HOME}
RUN sudo rm -rf spdk

RUN sudo sh -c "printf 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib\n' &>> ${HOME}/.bash_profile"
ENV LD_LIBRARY_PATH=/usr/local/lib

# Install NDN-DPDK with gcc v8.2.1
WORKDIR ${HOME}/go/src
RUN git clone https://github.com/usnistgov/ndn-dpdk.git
RUN sudo ln -s /usr/bin/gcc /usr/bin/gcc-7
WORKDIR ${HOME}/go/src/ndn-dpdk

# Use April 1st 2020 commit: 7194abef709e932e76853f6894abf825c8ca1d50
RUN git checkout 7194abef709e932e76853f6894abf825c8ca1d50

RUN sudo npm install -g jayson@3.2.0 tslint@6.1.0 typescript@3.8.3 protobufjs@6.8.8
RUN sudo npm install
RUN sed -ri 's/-Werror//g' mk/cflags.sh
RUN sed -ri 's/-Werror//g' mk/cbuild.sh
RUN make godeps
RUN make goget
RUN make cmds
RUN make tsc
RUN sudo make install
WORKDIR ${HOME}
