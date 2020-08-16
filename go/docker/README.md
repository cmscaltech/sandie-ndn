# sandie-ndn/go/docker

This [Dockerfile](Dockerfile) creates Docker image for running the pure-go main applications in this project and the [NDN-DPDK](https://github.com/usnistgov/ndn-dpdk) forwarder based on **CentOS 8.x** operating system.

### Build Docker

```console
user@cms:~$ cd sandie-ndn/go/docker
user@cms:~$ sudo docker build . -t sandiedockerimg  --network=host
```

### Run Docker container

```console
# needs root privileges and hugepages for NDN-DPDK forwarder
user@cms:~$ sudo docker run --ulimit core=-1 -d -it --privileged -v /sys/bus/pci/devices:/sys/bus/pci/devices -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev -v /mnt:/mnt --network=host sandiedockerimg
```
