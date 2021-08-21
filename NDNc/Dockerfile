# syntax=docker/dockerfile:experimental
FROM ubuntu:focal
FROM gcc:latest

ENV HOME /root
ENV BOOST_ROOT=/usr/include/boost

WORKDIR ${HOME}

RUN apt-get -y -qq update && \
    apt-get -y -qq install --no-install-recommends vim htop net-tools && \
    apt-get -y -qq install --no-install-recommends libicu-dev libbz2-dev libboost-all-dev ninja-build cmake \
    nlohmann-json3-dev

RUN git clone https://github.com/named-data/ndn-cxx.git --single-branch --branch ndn-cxx-0.7.1 --depth 1 && \
    cd ndn-cxx && ./waf configure && ./waf && ./waf install && ldconfig && cd ../

RUN git clone https://github.com/FDio/vpp.git --single-branch --branch v21.06 --depth=1 && \
    mkdir libmemif-build && cd libmemif-build && \
    cmake -G Ninja ../vpp/extras/libmemif && \
    ninja && $(which ninja) install && ldconfig && cd ../

WORKDIR ${HOME}
RUN rm -rf ndn-cxx vpp libmemif-build json

RUN g++ -v
RUN echo BOOST $(dpkg -s libboost-dev | grep 'Version') 
RUN echo $(cat /usr/local/include/ndn-cxx/version.hpp  | grep NDN_CXX_VERSION_STRING | head -1)
RUN echo $(cat /usr/local/include/libmemif.h | grep LIBMEMIF_VERSION | head -1)
RUN echo NLOHMANN JSON $(cat /usr/include/nlohmann/json.hpp | grep version | head -1)

RUN git clone https://github.com/cmscaltech/sandie-ndn.git
WORKDIR /root/sandie-ndn/NDNc
RUN make
