# sandie-ndn/go/cmd/sandie-consumer

This program acts as an [NDN](https://named-data.net/)  consumer able to copy files over the NDN network described by their pathname using `-input` flag.
It's using the pure-go consumer API from [`/internal/pkg/consumer`](../../internal/pkg/consumer).

This consumer is able to communicate on a network interface using AF_PACKET socket by specifying network interface name with `-i` flag
or connect to a local [NDN-DPDK](https://github.com/usnistgov/ndn-dpdk) forwarder by omitting `-i` flag.

## Usage example

#### Using AF_PACKET socket
```console
user@cms:~$ sudo $GOPATH/bin/sandie-consumer -i eth0 -local 9e:0a:a8:c5:d3:97 -remote 52:37:4f:e8:5a:80 -input /home/sandie/testbed/testfile
```

#### Using local NDN-DPDK forwarder

```console
# configure MTU 9000 on local network interface
user@cms:~$ ifconfig eth0 mtu 9000 up

# start NDN-DPDK forwarder
user@cms:~$ sudo MGMT=tcp://127.0.0.1:6345 ndnfw-dpdk -w 0000:5e:02.0 -- -initcfg @init-config.sample.yaml

# create face to local NDN-DPDK forwarder
user@cms:~$ jsonrpc2client -transport=tcp -tcp.addr=127.0.0.1:6345 'Face.Create' '{
  "scheme": "ether",
    "local": "9e:0a:a8:c5:d3:97",
    "remote": "52:37:4f:e8:5a:80",
    "portConfig": {
      "mtu": 9000
    }
}'

# insert prefix to fib
user@cms:~$ sudo ndndpdk-mgmtcmd fib insert /ndn/xrootd <face id:output of previous command>

# copy a file over NDN
user@cms:~$ $GOPATH/bin/sandie-consumer -input /home/sandie/testbed/testfile

```

Execute `$GOPATH/bin/sandie-consumer -h` to see additional flags.

## Logging

More information about how to enable different log levels can be found at [`/internal/pkg/logger`](../../internal/pkg/logger).
Example on how to enable all consumer DEBUG logs:
```console
user@cms:~$ export LOG_consumer=D LOG_consumerapp=D LOG_consumercmd=D
```
