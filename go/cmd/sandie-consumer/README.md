# sandie-ndn/go/cmd/sandie-consumer

//TODO Enable logging
```
$ ifconfig eth0 mtu 9000 up
# MGMT=tcp://127.0.0.1:6345 ndnfw-dpdk  --file-prefix=forwarder_consumer  -w 0000:5e:02.0 -- -initcfg @init-config.sample.yaml
```
```
$ jsonrpc2client -transport=tcp -tcp.addr=127.0.0.1:6345 'Face.Create' '{
  "scheme": "ether",
    "local": "9e:0a:a8:c5:d3:97",
    "remote": "52:37:4f:e8:5a:80",
    "portConfig": {
      "mtu": 9000
    }
}'

$ ndndpdk-mgmtcmd fib insert /ndn/xrootd <Face Id>
$ ndndpdk-mgmtcmd fib list
```

## Usage
memif:
```
user@cms:~# $GOPATH/bin/sandie-consumer
```
