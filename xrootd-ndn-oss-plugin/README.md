# The NDN based File System XRootD plugin component for Open Storage System and a suitable NDN producer

The NDN based File System component allows [XRootD](http://xrootd.org/) server to query the [Named Data Networking](https://named-data.net/) (NDN) network via the [Open Storage System](http://xrootd.org/doc/dev49/ofs_config.htm) (OSS) plugin layer. The plugin represents an NDN consumer that composes Interest packets for different system calls and waits for answers (Data packets). In this regard, a suitable NDN producer has been implemented which registers to the NFD and is able to answer requests from the XRootD OSS plugin.

At the moment the following system calls are supported: **open**, **fstat**, **close** and **read**.

## Build System

This project builds three separate entities: the [NDN based File System XRootD plugin](#the-ndn-based-file-system-xrootd-plugin), the [producer](#the-ndn-producer) and the [consumer](#the-ndn-consumer) embedded in the XRootD OSS plugin. The consumer application is build only for testing purposes.

Intall latest prerequisites from the [SANDIE repository](https://github.com/cmscaltech/sandie-ndn-repo).

In order to build and install the XRootD OSS plugin, the producer and the consumer from sources, use the following commands:

```console
root@cms:~# mkdir sandie-ndn/xrootd-ndn-oss-plugin/build
root@cms:~# cd sandie-ndn/xrootd-ndn-oss-plugin/build
root@cms:~# cmake ../
root@cms:~# make && make install
```

## The NDN based File System XRootD plugin

### Build and install from sources
In order to build and install the XRootD OSS plugin component from sources, use the following commands:

```consoler
root@cms:~# mkdir sandie-ndn/xrootd-ndn-oss-plugin/build
root@cms:~# cd sandie-ndn/xrootd-ndn-oss-plugin/build
root@cms:~# cmake ../
root@cms:~# make XrdNdnDFS && make install XrdNdnDFS
```

### Packaging
The NDN OSS XRootD plugin component is delivered as an RPM package.\
The latest release version can be found at [`xrootd-ndn-fs-<version>-<rn>.el7.x86_64.rpm`](https://github.com/cmscaltech/sandie-ndn-repo/tree/master/RPMS/x86_64). It contains the following files:

1. [**/etc/xrootd/xrootd-ndn.sample.cfg**](rpms/XrdNdnFS/xrootd-ndn.sample.cfg) This is a simple sample configuration file to run XRootD with the NDN based OSS plugin as data server. It specifies **libXrdNdnFS.so** as the OSS plugin component for XRootD and explains and sets default parameters for the shared library.

2. [**/etc/xrootd/xrootd-ndn.cfg**](rpms/XrdNdnFS/xrootd-ndn.sample.cfg) Copy of the sample configuration file for running XRootD as a systemd service. If modified, this file will not be erased on uninstall, but saved with *.rpmsave* extension. If an existing file is in place before installing the package or updating an existing version, this file will be copied with extension *.rpmnew*.

3. **/usr/lib64/libXrdNdnFS.so** The NDN based OSS plugin for XRootD.

### Install from RPM: xrootd-ndn-fs-\<version>-\<rn>.el7.x86_64.rpm

The XRootD OSS plugin component package requires the following dependencies:
* [XRootD Server 5.0.0](https://opensciencegrid.org/docs/data/xrootd/overview/) or later.
* [NFD 0.6.6](https://github.com/cmscaltech/sandie-ndn-repo/tree/master/RPMS/x86_64/nfd-0.7.0) or later.

You can download the latest NDN XRootD OSS plugin component package from the [delivery location](https://github.com/cmscaltech/sandie-ndn-repo/tree/master/RPMS/x86_64) and install it using the *rpm* command line tool as follows (this example is for version 0.1.2):
```console
root@cms:~# rpm -ivh xrootd-ndn-fs-0.1.2-1.el7.x86_64.rpm
```

### Usage and testing

In order to use the NDN OSS XRootD plugin you have to add the following line to your XRootD configuration file: `ofs.osslib /usr/lib64/libXrdNdnFS.so` or use the configuration file provided by the RPM package ([/etc/xrootd/xrootd.sample.ndnfd.cfg](rpms/XrdNdnFS/xrootd-ndn.sample.cfg)). After this, you can run XRootD but you have to make sure that NFD is running and over the network is an active [producer](#the-ndn-producer) that is capable to respond to XRootD requests.

The shared library accepts arguments, but are not mandatory, for configuring the embedded consumer. These arguments are:
* **interestlifetime**: The lifetime of Interest packets
* **loglevel**: The log level. Available options: [NONE, TRACE, INFO, WARN, DEBUG, ERROR, FATAL](https://named-data.net/doc/ndn-cxx/current/manpages/ndn-log.html)
* **pipelinesize**: The number of concurrent Interest packets expressed at one time in the fixed window size pipeline
* e.g.: `ofs.osslib /usr/lib64/libXrdNdnFS.so pipelinesize 64 interestlifetime 512 loglevel INFO`


## The NDN producer

### Build and install from sources

In order to build and install the NDN producer from sources, use the following commands:

```console
root@cms:~# mkdir sandie-ndn/xrootd-ndn-oss-plugin/build
root@cms:~# cd sandie-ndn/xrootd-ndn-oss-plugin/build
root@cms:~# cmake ../
root@cms:~# make xrdndn-producer && make install xrdndn-producer
```

### Packaging

The NDN producer is delivered as an RPM package. The latest release version can be found at [`xrdndn-producer-.<version>-<rn>.el7.x86_64.rpm`](https://github.com/cmscaltech/sandie-ndn-repo/tree/master/RPMS/x86_64). It contains the following files:

1. **/usr/bin/xrdndn-producer** This is the NDN producer that is able to respond to the NDN OSS XRootD plugin Interests.

2. [**/usr/lib/systemd/system/xrdndn-producer.service**](rpms/xrdndn-producer/xrdndn-producer.service) The NDN producer is installed as a systemd service on CentOS 7/RHEL. This file represents the configuration file for the service and it runs the */usr/bin/xrdndn-producer* binary.

3. [**/etc/rsyslog.d/xrdndn-producer-rsyslog.conf**](rpms/xrdndn-producer/xrdndn-producer-rsyslog.conf) The NDN producer service uses rsyslog for logging and this file configures rsyslogd to save all logs for *xrdndn-producer identifier* at a default location: **/var/log/xrdndn-producer/xrdndn-producer.log**.

4. [**/etc/ndn/nfd.xrdndn-producer.conf.sample**](rpms/xrdndn-producer/nfd.xrdndn-producer.conf.sample) Simple sample configuration file for NFD.

5. [**/etc/xrdndn-producer/xrdndn-producer.sample.cfg**](rpms/xrdndn-producer/xrdndn-producer.sample.cfg) Sample configuration file configuring xrdndn-producer systemd service.

6. [**/etc/xrdndn-producer/xrdndn-producer.cfg**](rpms/xrdndn-producer/xrdndn-producer.sample.cfg) Copy of the sample configuration file. If modified, this file will not be erased on uninstall, but saved with *.rpmsave* extension. If an existing file is in place before installing the package or updating an existing version, this file will be copied with extension *.rpmnew*. If removed, xrdndn-producer systemd service will run with default parameters. For more information about each individual parameter, please consult the actual sample configuration file.

### Install from RPM: xrdndn-producer-.\<version>-\<rn>.el7.x86_64.rpm

The NDN producer requires the following dependencies:
* [NFD 0.7.0](https://github.com/cmscaltech/sandie-ndn-repo/tree/master/RPMS/x86_64/nfd-0.7.0) or later.

You can donwload the latest NDN producer package from the [delivery location](https://github.com/cmscaltech/sandie-ndn-repo/tree/master/RPMS/x86_64) and install it using the *rpm* command line tool as follows (this example is for version 0.1.2):
```console
root@cms:~# rpm -ivh xrdndn-producer-0.1.2-1.el7.x86_64.rpm
```

### Usage and testing

The producer is installed as a systemd service. It requires a local NDN forwarder in order to connect to it and register prefixes over NDN network. You can use *service* command line tool to see status, start, restart or stop the service:

```console
root@cms:~# service xrdndn-producer status
root@cms:~# service xrdndn-producer start
root@cms:~# service xrdndn-producer restart
root@cms:~# service xrdndn-producer stop
```

Consult the [/etc/xrdndn-producer/xrdndn-producer.cfg](rpms/xrdndn-producer/xrdndn-producer.sample.cfg) configuration file to change default parameters or the log level.

Although the producer is installed as a systemd service, it can also be used for testing purposes. Because of this, the *xrdndn-producer* binary under the */usr/bin/* directory accepts command line arguments:

```console
root@cms:~# xrdndn-producer -h
```


## The NDN consumer

As stated, the NDN base File System plugin component for XRootD OSS uses an NDN consumer that is able to translate different system calls into NDN Interest packets and send them over the network. For testing purposes only, the consumer can be installed as a standalone application.

### Build and install from sources

In order to build and install the NDN consumer from sources, use the following commands:

```console
root@cms:~# mkdir sandie-ndn/xrootd-ndn-oss-plugin/build
root@cms:~# cd sandie-ndn/xrootd-ndn-oss-plugin/build
root@cms:~# cmake ../
root@cms:~# make xrdndn-consumer && make install xrdndn-consumer
```

### Packaging

The NDN consumer is also delivered as an RPM package.
The latest release version can be found at [`xrdndn-consumer-<version>-<rn>.el7.x86_64.rpm`](https://github.com/cmscaltech/sandie-ndn-repo/tree/master/RPMS/x86_64). It contains only **/usr/bin/xrdndn-consumer** file, which is the binary.

### Install from RPM: xrdndn-consumer-.\<version>-\<rn>.el7.x86_64.rpm

The NDN consumer requires the following dependencies:
* [NFD 0.7.0](https://github.com/cmscaltech/sandie-ndn-repo/tree/master/RPMS/x86_64/nfd-0.7.0) or later.

You cand donwload the latest package from the delivery location and install it using the *rpm* command line tool as follows (this example is for version 0.1.4):

```console
root@cms:~# rpm -ivh xrdndn-consumer-0.1.4-1.el7.x86_64.rpm
```

### Usage and testing

The NDN consumer is installed as an application under */usr/bin/* directory. For the moment, it is used for testing only and it's able to request files over NDN network to a local or remote Producer. In order to see the different command line arguments it accepts run the following command in a terminal:

```console
root@cms:~# xrdndn-consumer -h
```
