# syntax=docker/dockerfile:experimental
FROM ubuntu:jammy

# add user for running the xrootd
ARG UNAME=xrootd
ARG UID=2010
ARG GID=2010
RUN groupadd -g $GID -o $UNAME
RUN useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME
RUN usermod -aG root xrootd

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles
ENV HOME /root
ENV BOOST_ROOT=/usr/include/boost

WORKDIR ${HOME}

RUN apt-get -y -qq update --allow-releaseinfo-change && \
    apt-get -y -qq install --no-install-recommends \
        tzdata \
        git-all \
        build-essential \
        cmake pkg-config \
        libkrb5-dev \
        libsqlite3-dev \
        libcurl4-nss-dev \
        vim \
        htop \
        net-tools \
        libicu-dev \
        libbz2-dev \
        libboost-all-dev \
        ninja-build cmake \
        nlohmann-json3-dev \
        openssh-server \
        apache2 \
        supervisor \
        xrootd-client \
        libxrootd-dev \
        libxrootd-client-dev \
        xrootd-server \
        xrootd-server-plugins \
        libxrootd-server-dev \
        libglobus-gsi-credential1 \
        libglobus-gsi-cert-utils0 \
        liblcmaps-dev \
        voms-dev \
        git-all

RUN git clone https://github.com/opensciencegrid/xrootd-lcmaps.git  --single-branch --branch v1.7.8 --depth 1 && \
    cd xrootd-lcmaps && sed -i '1s/^/#include <cstring>\n /' src/XrdHttpLcmaps.cc && \
    cmake . && make -j4 && make install && \
    mkdir -p /usr/lib64/ && cp libXrdLcmaps-5.so /usr/lib64/libXrdLcmaps.so && cd ../

RUN git clone https://github.com/named-data/ndn-cxx.git --single-branch --branch ndn-cxx-0.8.0 --depth 1 && \
    cd ndn-cxx && ./waf configure && ./waf && ./waf install && ldconfig && cd ../

RUN git clone https://github.com/FDio/vpp.git --single-branch --branch v23.02-rc0 --depth=1 && \
    mkdir libmemif-build && cd libmemif-build && \
    cmake -G Ninja ../vpp/extras/libmemif && \
    ninja && $(which ninja) install && ldconfig && cd ../

# cleanup
WORKDIR ${HOME}
RUN rm -rf ndn-cxx vpp libmemif-build json xrootd-lcmaps

# print version of all NDNc dependencies
RUN g++ -v
RUN echo BOOST $(dpkg -s libboost-dev | grep 'Version') 
RUN echo $(cat /usr/local/include/ndn-cxx/version.hpp  | grep NDN_CXX_VERSION_STRING | head -1)
RUN echo $(cat /usr/local/include/libmemif.h | grep LIBMEMIF_VERSION | head -1)
RUN echo NLOHMANN JSON $(cat /usr/include/nlohmann/json.hpp | grep version | head -1)

RUN git clone https://github.com/cmscaltech/sandie-ndn.git
RUN mkdir -p /root/sandie-ndn/NDNc/build
WORKDIR /root/sandie-ndn/NDNc/build
RUN cmake -DCMAKE_BUILD_TYPE=Release ../ && make -j16 && make install
RUN ldconfig /usr/lib64
WORKDIR ${HOME}

RUN mkdir -p /var/log/xrootd/ndn/
RUN touch /var/log/xrootd/ndn/xrootd.log
RUN chown -R xrootd:xrootd /var/log/xrootd/
RUN cp ${HOME}/sandie-ndn/NDNc/docker/xrootd-ndn.sample.cfg /etc/xrootd/xrootd-ndn.cfg
