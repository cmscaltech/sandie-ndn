# sandie-ndn/go/internal/pkg/consumer

This is the SANDIE pure-go consumer API. It is developed with the low-level APIs in [NDN-DPDK](https://github.com/usnistgov/ndn-dpdk/tree/master/ndn) and related packages.

Available API methods:
* **Stat**: get *FileInfo* (internal consumer struct) for a file in the NDN network.
* **ReadAt**: get a chunk of bytes at an offset from a file in the NDN network.

Interest packets expressed by this API are under **/ndn/xrootd** prefix. Maximum payload size is **6144B**.

For high-performance throughput the consumer API will connect to the local NDN-DPDK forwarder for sharing memory and transmitting and receiving packets over the NDN network.
It is also able to connect to a remote NDN entity: forwarder, producer, using AF_PACKET socket but with low performance.

Check the [`/internal/app/consumer`](../../app/consumer) source code as an example on how to use this API.

## Logging

More information about how to enable different log levels can be found at [`/internal/pkg/logger`](../logger).
Example on how to activate FATAL log level for consumer API:

```console
user@cms:~$ export LOG_consumer=F
```