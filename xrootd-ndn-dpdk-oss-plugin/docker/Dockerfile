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
RUN sudo yum -y install CUnit-devel bind-utils vim htop git wget tar rpm-build rake doxygen\
        cmake cmake3 make tcsh lsof tk tcl autoconf automake perl perl-GD elfutils-libelf-devel \
        openssl-devel yamllint libtool socat glibc-devel.i686 glibc-devel gcc-gfortran kernel-rpm-macros \
        libfdt numad numactl-devel numactl-libs libpcap-devel libbsd-devel jansson-devel nvme-cli \
        createrepo libaio-devel bzip2-devel libffi-devel pciutils pandoc meson ninja-build libnl3-devel \
        libudev-devel pkgconfig libuuid-devel nasm clang python3-distutils-extra \
        java-1.8.0-openjdk kernel-devel kernel-headers gdb

RUN sudo pip3 install ninja meson -U

# Install lcov 1.14
RUN sudo rpm -i https://netix.dl.sourceforge.net/project/ltp/Coverage%20Analysis/LCOV-1.14/lcov-1.14-1.noarch.rpm

# Install Node.js 12.x
RUN curl -sL https://rpm.nodesource.com/setup_12.x | sudo bash -
RUN sudo yum install -y nodejs

# Install Hadoop v3.2.1
ENV JAVA_HOME=/usr/lib/jvm/jre-1.8.0
RUN wget http://www.trieuvan.com/apache/hadoop/common/hadoop-3.2.1/hadoop-3.2.1.tar.gz
RUN tar xvzf hadoop-3.2.1.tar.gz && sudo mkdir -p /usr/local/hadoop && sudo mv hadoop-3.2.1/* /usr/local/hadoop && rm -rf hadoop-3.2.1 && rm hadoop-3.2.1.tar.gz
ENV HADOOP_INSTALL=/usr/local/hadoop
ENV PATH=$PATH:$HADOOP_INSTALL/bin:$HADOOP_INSTALL/sbin
ENV HADOOP_MAPRED_HOME=$HADOOP_INSTALL
ENV HADOOP_COMMON_HOME=$HADOOP_INSTALL
ENV HADOOP_HDFS_HOME=$HADOOP_INSTALL
ENV YARN_HOME=$HADOOP_INSTALL
ENV HADOOP_COMMON_LIB_NATIVE_DIR=$HADOOP_INSTALL/lib/native
ENV HADOOP_OPTS="-Djava.library.path=$HADOOP_INSTALL/lib/native"
ENV HADOOP_USER=hdfs
RUN sudo groupadd -r hadoop && sudo groupadd -r $HADOOP_USER && sudo useradd -r -g $HADOOP_USER -G hadoop $HADOOP_USER
RUN sudo chown -R $HADOOP_USER:hadoop $HADOOP_INSTALL
COPY hdfs_config/* $HADOOP_INSTALL/etc/hadoop/
RUN sudo  sed -ri "/export JAVA_HOME=/ c\export JAVA_HOME=$JAVA_HOME" $HADOOP_INSTALL/etc/hadoop/hadoop-env.sh

# Install jsonrpc2client 1.1.3
RUN set -o pipefail && curl -sfL https://github.com/powerman/rpc-codec/releases/download/v1.1.3/jsonrpc2client-linux-x86_64 | \
        sudo install /dev/stdin /usr/local/bin/jsonrpc2client

# Install Golang 1.14
RUN set -o pipefail && curl -L https://dl.google.com/go/go1.14.linux-amd64.tar.gz | sudo tar -C /usr/local -xz
RUN mkdir -p ${HOME}/go/src/
RUN sudo chown -R ${USER}:${USER} ${HOME}/go
ENV GOROOT=/usr/local/go
ENV GOPATH=${HOME}/go
ENV PATH=/usr/local/go/bin/:${HOME}/go/bin/:/usr/local/lib/:/usr/local/bin/:/usr/local/sbin:/usr/sbin:/usr/bin:/sbin:/bin

# Install go-bindata
RUN go get github.com/jteeuwen/go-bindata
WORKDIR $GOPATH/src/github.com/jteeuwen/go-bindata/go-bindata
RUN go build && sudo mv go-bindata $GOROOT/bin/
WORKDIR ${HOME}

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
ARG MLNX_OFED=MLNX_OFED_LINUX-5.0-2.1.8.0-rhel8.1-x86_64
RUN set -o pipefail && curl -L http://content.mellanox.com/ofed/MLNX_OFED-5.0-2.1.8.0/${MLNX_OFED}.tgz | tar -C . -xz
WORKDIR ${HOME}/${MLNX_OFED}
RUN set -o pipefail && export TERM=linux && sudo ./mlnxofedinstall -vvv --force --dpdk --add-kernel-support --kernel-sources /usr/src/kernels/`uname -r`
WORKDIR ${HOME}
RUN rm -rf ${MLNX_OFED}

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

# Install DPDK v20.05
RUN set -o pipefail && curl -sL https://static.dpdk.org/rel/dpdk-20.05.tar.xz | tar -C . -xJ
WORKDIR ${HOME}/dpdk-20.05
RUN sed -ri 's,(CONFIG_RTE_BUILD_SHARED_LIB=).*,\1y,' ./config/common_base && \
        sed -ri 's,(CONFIG_RTE_LIBRTE_BPF_ELF=).*,\1y,' ./config/common_base && \
        sed -ri 's,(CONFIG_RTE_LIBRTE_PMD_OPENSSL=).*,\1y,' ./config/common_base && \
        sed -ri 's,(CONFIG_RTE_LIBRTE_MLX5_PMD=).*,\1y,' ./config/common_base && \
        sed -ri 's,(RTE_MACHINE=).*,\1y,' ./config/common_base && \
        sed -ri 's,(CONFIG_RTE_IBVERBS_LINK_DLOPEN=).*,\1y,' ./config/common_base
RUN CC=gcc meson -Dtests=false --libdir=lib build
WORKDIR ${HOME}/dpdk-20.05/build
RUN sudo ninja install
RUN sudo find /usr/local/lib -name 'librte_*.a' -delete
RUN sudo ldconfig
WORKDIR ${HOME}
RUN sudo rm -rf dpdk-20.05
ENV PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig

# Install SPDK v20.04.1
RUN set -o pipefail && curl -sL https://github.com/spdk/spdk/archive/v20.04.1.tar.gz | tar -C . -xz
WORKDIR ${HOME}/spdk-20.04.1
RUN sed -ri '/SPDK_MEMPOOL_DEFAULT_CACHE_SIZE/ c\#define SPDK_MEMPOOL_DEFAULT_CACHE_SIZE 512' include/spdk/env.h
RUN CC=gcc LDFLAGS="-lnuma -lbsd" ./configure --enable-debug --disable-tests --with-shared --with-dpdk=/usr/local --without-vhost --without-isal --without-fuse
RUN make -j16
RUN sudo make install
RUN sudo ldconfig
WORKDIR ${HOME}
RUN sudo rm -rf spdk-20.04.1
ENV LD_LIBRARY_PATH=/usr/local/lib

# Configure global environment variables
RUN sudo sh -c "printf '\
export GOROOT=${GOROOT}\n\
export GOPATH=${GOPATH}\n\
export PATH=\$PATH:${PATH}\n\
export PKG_CONFIG_PATH=\$PKG_CONFIG_PATH:${PKG_CONFIG_PATH}\n\
\n\
export JRE_HOME=${JAVA_HOME}\n\
export HADOOP_HOME=${HADOOP_INSTALL}\n\
export PATH=\$PATH:\${HADOOP_HOME}/bin:\${HADOOP_HOME}/sbin\n\
export LIBRARY_PATH=\$LIBRARY_PATH:\${HADOOP_HOME}/lib/native:\${JRE_HOME}/lib/amd64/server\n\
export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:${LD_LIBRARY_PATH}:\$LIBRARY_PATH\n\
export CLASSPATH=\$CLASSPATH:\$(\${HADOOP_HOME}/bin/hadoop classpath --glob)\n\
\n\
ulimit -u unlimited\
' > /etc/profile.d/container.sh"

# Install SANDIE consumer and producer applications and NDN-DPDK
ENV HADOOP_HOME=$HADOOP_INSTALL
ENV LIBRARY_PATH=$LIBRARY_PATH:$HADOOP_HOME/lib/native:$JAVA_HOME/lib/amd64/server
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LIBRARY_PATH

RUN mkdir ${GOPATH}/src/github.com/usnistgov
WORKDIR ${GOPATH}/src/github.com/usnistgov
RUN git clone https://github.com/usnistgov/ndn-dpdk.git
WORKDIR ${GOPATH}/src/github.com/usnistgov/ndn-dpdk
# Use July 16th 2020 commit: 654a7ac3589b02b4adf3683b56767ed63e202423
RUN git checkout 654a7ac3589b02b4adf3683b56767ed63e202423
RUN npm install webpack@1.11.0
RUN npm install
ENV BPFCC=clang-9
RUN RELEASE=1 make
RUN sudo make install

WORKDIR ${GOPATH}/src
RUN git clone https://github.com/cmscaltech/sandie-ndn.git
WORKDIR ${GOPATH}/src/sandie-ndn/xrootd-ndn-dpdk-oss-plugin
RUN make
WORKDIR ${HOME}
