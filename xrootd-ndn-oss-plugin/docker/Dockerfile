FROM centos:7.7.1908
ENV HOME /root

RUN yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
RUN rpm --import http://linuxsoft.cern.ch/wlcg/RPM-GPG-KEY-wlcg
RUN yum -y install http://linuxsoft.cern.ch/wlcg/centos7/x86_64/wlcg-repo-1.0.0-1.el7.noarch.rpm
RUN yum -y install alicexrdplugins

RUN yum -y install sudo vim htop git curl.x86_64 bind-utils.x86_64 \
        redhat-lsb.x86_64 bzip2.x86_64 libicu libpcap initscripts \
        libstdc++-devel net-tools cmake3.x86_64 xrootd-client.x86_64 \
        xrootd-client-devel.x86_64 xrootd-client-libs.x86_64 \
        xrootd-devel.x86_64 xrootd-libs.x86_64 xrootd-server.x86_64 \
        xrootd-server-devel.x86_64 xrootd-server-libs.x86_64 centos-release-scl
RUN yum -y install devtoolset-8-gcc*
RUN echo 'source scl_source enable devtoolset-8' >> ~/.bashrc

# Install NDN Software Stack
WORKDIR ${HOME}
RUN git clone https://github.com/cmscaltech/sandie-ndn.git
WORKDIR ${HOME}/sandie-ndn/packaging/RPMS/x86_64/
RUN rpm -i boost1_69_0/boost-license1_69_0-1.69.0-1.x86_64.rpm \
        boost1_69_0/libboost_* boost1_69_0/boost-devel-1.69.0-1.x86_64.rpm \
        ndn-cxx-0.6.6/libndn-cxx-* ndn-tools-0.6.4/ndn-tools-0.6.4-10.el7.x86_64.rpm \
        nfd-0.6.6/nfd-0.6.6-24.el7.x86_64.rpm xrdndn-consumer-0.2.0-4.el7.x86_64.rpm \
        xrdndn-producer-0.2.0-4.el7.x86_64.rpm xrootd-ndn-fs-0.2.0-4.el7.x86_64.rpm
WORKDIR ${HOME}/sandie-ndn
RUN rm -rf packaging cms_topology pub_keys sc19 xrootd-ndn-dpdk-oss-plugin

# Build xrdndn-consumer xrdndn-producer xrdndn-fs
RUN mkdir -p ${HOME}/sandie-ndn/xrootd-ndn-oss-plugin/build
WORKDIR ${HOME}/sandie-ndn/xrootd-ndn-oss-plugin/build
RUN source ~/.bashrc && cmake3 ../
RUN make

WORKDIR ${HOME}
