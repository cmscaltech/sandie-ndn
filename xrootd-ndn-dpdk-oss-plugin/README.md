# NDN-DPDK Based Consumer-Producer Application for copying files

## Available support
At the moment the following system calls are supported: **open**.

## Developer Guide

1. Follow installation steps for [NDN-DPDK](https://github.com/usnistgov/ndn-dpdk)

2. Clone repository next to NDN-DPDK one. Your file hierarchy should look like this:
```bash
go
|-- bin
|-- pkg
|   [...]
`-- src
    |-- [...]
    |-- ndn-dpdk
    |	[...]
    `-- sandie-ndn
       	|- [...]
        |-- xrootd-ndn-dpdk-oss-plugin
        |   |-- xrdndndpdk-common
        |   |-- xrdndndpdkproducer
        |   `-- xrdndndpdkproducer-cmd
        `-- [...]
```

3. Use [Makefile](./Makefile) to build *producer* and *consumer*

## Usage

### Consumer
Edit yaml configuration file to specify the path to the file to be copied using *filepath* argument.

```bash
MGMT=tcp://127.0.0.1:6345 ./xrdndndpdkconsumer-cmd -w 0000:8f:00.0 -- -initcfg @init-config-client.yaml -tasks @xrdndndpdkconsumer.yaml

or using raw socket:

MGMT=tcp://127.0.0.1:6345 ./xrdndndpdkconsumer-cmd --vdev net_af_packetL,iface=enp143s0  -- -initcfg @init-config.yaml -tasks @xrdndndpdkconsumer-raw-socket.yaml
```

### Producer
```bash
MGMT=tcp://127.0.0.1:6345 ./xrdndndpdkproducer-cmd -w 0000:5e:00.0 -- -initcfg @init-config.yaml -tasks @xrdndndpdkproducer.yaml

or using raw socket:

MGMT=tcp://127.0.0.1:6345 ./xrdndndpdkproducer-cmd --vdev net_af_packetL,iface=ens2 -- -initcfg @init-config.yaml -tasks @xrdndndpdkproducer-raw-socket.yaml
```

## Logging

Active different log levels for consumer and producer:
```bash
[]# export LOG_Xrdndndpdkconsumer=D
[]# export LOG_Xrdndndpdkproducer=D
```

Available log levels:

* **V**: VERBOSE level (C only)
* **D**: DEBUG level
* **I**: INFO level (default)
* **W**: WARNING level
* **E**: ERROR level
* **F**: FATAL level
* **N**: disabled (in C), PANIC level (in Go)
