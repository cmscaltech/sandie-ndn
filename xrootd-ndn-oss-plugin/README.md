# A NDN based filesystem plugin for XRootD and a suitable NDN Producer

This plugin allows XRootD server to talk directly to NDN via the OSS plugin layer. The plugin represents a NDN Consumer which composes interests packages for different system calls and waits for answers (data packages). In this regard, a suitable NDN Producer has been implemented which registers to NFD and is able to answer to request from the XRootD OSS plugin.

For the moment, the following system calls are supported: **open**, **fstat**, **close**, **read**.

## Build System

This projects uses CMake to handle the build process. It should build fine with version 3.5 or higher.

## Build and install the source
```bash 
user@cms:~# mkdir sandie-ndn/xrootd-ndn-fs-plugin/build 
user@cms:~# cd sandie-ndn/xrootd-ndn-fs-plugin/build
user@cms:~# cmake ../
user@cms:~# make && make install
```
## Enable and run

To enable the XRootD oss plugin, add the following line to your XRootD build: ```ofs.osslib /usr/lib64/libXrdNdnFS.so``` or alternatively use the configuration file provided in this repository: **rpms/xrootd.sample.ndnfd.cfg**.

To run the the NDN Producer use the following commands:
```bash
user@cms:~# nfd-start
user@cms:~# xrdndn-producer
```

## Logging using ndn-log
By default, only **FATAL** errors will be printed to stdout. Follow [ndn-log](https://named-data.net/doc/ndn-cxx/current/manpages/ndn-log.html) to read about the different levels of logging and how to enable them. To enable all levels of logging run the following command:
```bash
user@cms:~# export NDN_LOG="xrdndnconsumer*=TRACE:xrdndnproducer*=TRACE"
```

The above command suffice for the **xrdndn-producer**, however, in order to print the output from the consumer, you need to also add the following command to your XRootD build: ```ofs.trace all```.
