# NDN-DPDK Based Consumer-Producer Application For Copying Files

## Available support
The consumer application is able to request a list of files to be copied over NDN using Interest packets. The producer is able to respond to Interests sent by consumer by replying with Data packets.

## Developer Guide

1. Clone this repository to: **$GOPATH/src/**
2. Run `cd $GOPATH/src/sandie-ndn/xrootd-ndn-dpdk-oss-plugin && make godeps`. This will download all Go dependencies including [NDN-DPDK Git repository](https://github.com/usnistgov/ndn-dpdk)
3. Go to `$GOPATH/src/github.com/usnistgov/ndn-dpdk` and checkout to revision **dfeaab3d**. Follow the instruction on how to build NDN-DPDK from [official Git page](https://github.com/usnistgov/ndn-dpdk). You can also use the [Dockerfile](./docker/Dockerfile) as guideline on how to build NDN-DPDK and all required dependencies on CentOS 8
4. Run `cd $GOPATH/src/sandie-ndn/xrootd-ndn-dpdk-oss-plugin`
5. Run `make`. This will build both consumer and producer application into *$GOPATH/bin*

## Usage

Use [init-config.sample.yaml](./init-config.sample.yaml), [init-config-consumer.sample.yaml](./init-config-consumer.sample.yaml) and [init-config-consumer.sample.yaml](./init-config-consumer.sample.yaml) files in order to run the consumer and producer applications. Example:

```bash
./xrdndndpdkconsumer-cmd -w 0000:8f:00.0 -- -initcfg @init-config.sample.yaml -initcfgconsumer @init-config-consumer.sample.yaml
./xrdndndpdkproducer-cmd -w 0000:5e:00.0 -- -initcfg @init-config.sample.yaml -initcfgproducer @init-config-consumer.sample.yaml
```

## Logging

Active different log levels for consumer and producer:
```bash
[]# export LOG_Xrdndndpdkconsumer=I
[]# export LOG_Xrdndndpdkproducer=I
[]# export LOG_Xrdndndpdkcommon=I
[]# export LOG_Xrdndndpdkfilesystem=I
```

Available log levels:

* **V**: VERBOSE level (C only)
* **D**: DEBUG level
* **I**: INFO level (default)
* **W**: WARNING level
* **E**: ERROR level
* **F**: FATAL level
* **N**: disabled (in C), PANIC level (in Go)
