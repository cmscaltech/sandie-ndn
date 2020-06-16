# NDN-DPDK Based Consumer-Producer Application For Copying Files

## Available support
At the moment the following system calls are supported: **open**, **fstat**, **read**.

## Developer Guide

1. Clone [NDN-DPDK](https://github.com/usnistgov/ndn-dpdk) repository at revision: **c1c73f749675c** and follow installation steps

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

3. `export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig`

4. `export HADOOP_HOME=<path to Hadoop install dir>`

5. Use [Makefile](./Makefile) to build *producer* and *consumer*

## Usage

TODO: Configuration for high-troughput (yaml file and hardware/software setup)

### Consumer
Edit yaml configuration file to specify the path to the file to be copied using *filepath* argument.

```bash
MGMT=tcp://127.0.0.1:6345 ./xrdndndpdkconsumer-cmd -w 0000:8f:00.0 -- -initcfg @init-config-client.yaml -tasks @xrdndndpdkconsumer.yaml
```

### Producer
```bash
MGMT=tcp://127.0.0.1:6345 ./xrdndndpdkproducer-cmd -w 0000:5e:00.0 -- -initcfg @init-config.yaml -initcfgproducer @xrdndndpdkproducer-sample.yaml
```

## Logging

Active different log levels for consumer and producer:
```bash
[]# export LOG_Xrdndndpdkconsumer=D
[]# export LOG_Xrdndndpdkproducer=D
[]# export LOG_Xrdndndpdkutils=D
[]# export LOG_Xrdndndpdkfilesystem=V
```

Available log levels:

* **V**: VERBOSE level (C only)
* **D**: DEBUG level
* **I**: INFO level (default)
* **W**: WARNING level
* **E**: ERROR level
* **F**: FATAL level
* **N**: disabled (in C), PANIC level (in Go)
