# A NDN based filesystem plugin for XRootD and a suitable NDN Producer

This plugin allows [XRootD](http://xrootd.org/) server to query the NDN via the OSS plugin layer. The plugin represents an NDN Consumer that composes interests packages for different system calls and waits for answers (Data packet). In this regard, a suitable NDN Producer has been implemented which registers to NFD and is able to answer requests from the XRootD OSS plugin.

For the moment, the following system calls are supported: **open**, **fstat**, **close**, **read**.

## Build System

This project uses CMake to handle the build process. It should build fine with version 3.5 or higher.

In order to build and install both the Producer and the XRootD OSS plugin from sources, use the following commands:

```bash
root@cms:~# mkdir sandie-ndn/xrootd-ndn-fs-plugin/build
root@cms:~# cd sandie-ndn/xrootd-ndn-fs-plugin/build
root@cms:~# cmake ../
root@cms:~# make && make install
```

## The NDN based filesystem plugin for XRootD
### Build and install from sources
In order to build and install the XRootD OSS plugin from sources, use the following commands:

```bash
root@cms:~# mkdir sandie-ndn/xrootd-ndn-fs-plugin/build
root@cms:~# cd sandie-ndn/xrootd-ndn-fs-plugin/build
root@cms:~# cmake ../
root@cms:~# make XrdNdnDFS && make install XrdNdnDFS
```

### Packaging
The XRootD plugin is delivered as an RPM package.\
The latest release version can be found at [`xrootd-ndn-fs-<version>-1.el7.x86_64.rpm`](../packaging/RPMS/x86_64). It contains the following files:

1. [**/etc/xrootd/xrootd.sample.ndnfd.cfg**](rpms/XrdNdnFS/xrootd.sample.ndnfd.cfg) This represents a basic configuration file for XRootD. It specifies **libXrdNdnFS.so** as the OSS implementation for XRootD and also enables all levels of logging for *ofs* and *xrd* modules. You can use, modify or ignore this file. To use it start XRootD as follows:
    ```bash
    user@cms:~$ xrootd -c /etc/xrootd/xrootd.sample.ndnfd.cfg
    ```

2. **/usr/lib64/libXrdNdnFS.so** This is the XRootD OSS plugin. As stated before, it contains an NDN Consumer that sends interest packages over the NDN network for different system calls. To use it, you have to add the following line to your XRootD configuration file: `ofs.osslib /usr/lib64/libXrdNdnFS.so` or use the configuration file provided by the RPM package([*/etc/xrootd/xrootd.sample.ndnfd.cfg*](rpms/XrdNdnFS/xrootd.sample.ndnfd.cfg)).

### Install from RPM: xrootd-ndn-fs-\<version>-1.el7.x86_64.rpm

The XRootD plugin requires the following dependencies:
* [XRootD Server 4.8.0](http://opensciencegrid.org/docs/data/install-xrootd/) or later.
* [NFD 0.6.2](../packaging/RPMS/x86_64/nfd-0.6.2-1.el7.centos.x86_64.rpm) or later.

You can install the XRootD plugin by downloading the latest package from the [delivery location](../packaging/RPMS/x86_64) and install it using the *rpm* command line tool as follows (this example is for version 0.1.2):
```bash
root@cms:~# rpm -ivh xrootd-ndn-fs-0.1.2-1.el7.x86_64.rpm
```

### Usage and testing

In order to use the NDN based filesystem XRootD plugin you have to add the following line to your XRootD configuration file: `ofs.osslib /usr/lib64/libXrdNdnFS.so` or use the configuration file provided by the RPM package([*/etc/xrootd/xrootd.sample.ndnfd.cfg*](rpms/XrdNdnFS/xrootd.sample.ndnfd.cfg)). After this you can run XRootD but you have to make sure that NFD is running and over the network is an [NDN Producer](#the-ndn-producer) that is capable to respond to XRootD requests.

Because the plugin contains an NDN Consumer that registers to the NDN logging module with the name **xrdndnconsumer**, you can enable logging by adding the the following line to your XRootD configuration file: `ofs.trace all` and running the following command:
```bash
user@cms:~$ export NDN_LOG="xrdndnconsumer*=TRACE"
````

You can read more about the different levels of logging at [section](#logging-using-ndn-log).

## The NDN Producer
### Build and install from sources
In order to build and install the NDN Producer from sources, use the following commands:

```bash
root@cms:~# mkdir sandie-ndn/xrootd-ndn-fs-plugin/build
root@cms:~# cd sandie-ndn/xrootd-ndn-fs-plugin/build
root@cms:~# cmake ../
root@cms:~# make xrdndn-producer && make install xrdndn-producer
```

### Packaging

The NDN Producer is delivered as an RPM package.\
The latest release version can be found at [`xrdndn-producer-.<version>.el7.x86_64.rpm`](../packaging/RPMS/x86_64). It contains the following files:

1. **/usr/bin/xrdndn-producer** This is the NDN Producer that is able to respond to the NDN Consumer contained in the XRootD OSS plugin.

2. [**/usr/lib/systemd/system/xrdndn-producer.service**](rpms/xrdndn-producer/xrdndn-producer.service) The NDN Producer is installed as a systemd service on CentOS 7/RHEL. This file represents the configuration file for the service and it runs the */usr/bin/xrdndn-producer* binary.

3. [**/etc/sysconfig/xrdndn-producer**](rpms/xrdndn-producer/xrdndn-producer-environment) The NDN Producer registers to the NDN logging module with the name *xrdndnproducer*. Because of the way the NDN logging system works, NDN_LOG environment variable needs to be exported. More details about the different logging levels can be found at [section](#logging-using-ndn-log). This file contains a definition of the NDN_LOG environment variable and by default the **INFO** logging level is enabled. You can modify this file as you desire. To see logs you can use the *journalctl* command line tool as follows:
   ```bash
   root@cms:~# journalctl -l -u xrdndn-producer
   ```

4. [**/etc/rsyslog.d/xrdndn-producer-rsyslog.conf**](rpms/xrdndn-producer/xrdndn-producer-rsyslog.conf) The NDN Producer service uses rsyslog for logging and this file configures rsyslogd to save all logs for *xrdndn-producer* at a default location: **/var/log/xrdndn-producer.log**.

5. [**/etc/ndn/nfd.xrootd.conf.sample**](rpms/xrdndn-producer/nfd.xrootd.conf.sample) In order to run NFD you need a configuration file for it. For the moment, this file is simply the default NFD configuration file, but it can change in the future.

### Install from RPM: xrdndn-producer-.\<version>.el7.x86_64.rpm

The NDN Producer requires the following dependencies:
* [NFD 0.6.2](../packaging/RPMS/x86_64/nfd-0.6.2-1.el7.centos.x86_64.rpm) or later.

You can install the NDN Producer by downloading the latest package from the [delivery location](../packaging/RPMS/x86_64) and install it using the *rpm* command line tool as follows (this example is for version 0.1.2):
```bash
root@cms:~# rpm -ivh xrdndn-producer-0.1.2-1.el7.x86_64.rpm
```

### Usage and testing

The NDN Producer is installed as a systemd service. It requires NFD to run, in order for it to register all the prefixes it listens over the network. You can use *systemctl* command line tool to see status, start, restart or stop the service:

```bash
root@cms:~# systemctl status xrdndn-producer.service
root@cms:~# systemctl start xrdndn-producer.service
root@cms:~# systemctl stop xrdndn-producer.service
root@cms:~# systemctl restart xrdndn-producer.service
```
You can change to logging level of the Producer by modifying: **/etc/sysconfig/xrdndn-producer** file.

## Logging using ndn-log
By default, only **FATAL** errors will be printed to stdout. Follow [ndn-log](https://named-data.net/doc/ndn-cxx/current/manpages/ndn-log.html) to read about the different levels of logging and how to enable them. 

All the logging module names are defined in [xrdndn-logger.hh](./src/common/xrdndn-logger.hh) source file. For the moment there are only two modules defined: **xrdndnconsumer** (for NDN based filesystem XRootD plugin) and **xrdndnproducer** (for the NDN producer).

To enable all levels of logging for the XRootD plugin run the following command:
```bash
user@cms:~# export NDN_LOG="xrdndnconsumer*=TRACE"
```
and add the following line to your XRootD build: ```ofs.trace all```.

To enable all levels of logging for the NDN Producer you need to add the following line to */etc/sysconfig/xrdndn-producer* file: `NDN_LOG="xrdndnproducer*=TRACE"`.
