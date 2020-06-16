# NDN Dockerfile

This [Dockerfile](Dockerfile) creates Docker container for NDN-DPDK based [consumer and producer applications](../README.md) for XRootD OSS plugin based on CentOS 8.2.

## Build Docker image from Dockerfile

```bash
sudo yum update
sudo docker build . -t sandiendndpdk:1.0.9  --network=host

```

## Run Docker container from image

```bash

sudo docker run -d -it --privileged -v /sys/bus/pci/devices:/sys/bus/pci/devices -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev -v /mnt:/mnt --network=host sandiendndpdk:1.0.9

```

## Run Docker containers using SR-IOV specification

```bash

sudo docker pull rdma/container_tools_installer
sudo docker pull rdma/sriov-plugin
sudo docker run -v /usr/bin:/tmp --net=host rdma/container_tools_installer
sudo docker run -d --rm -v /run/docker/plugins:/run/docker/plugins -v /etc/docker:/etc/docker -v /var/run:/var/run --net=host --privileged rdma/sriov-plugin
sudo docker network create -d sriov --subnet=100.10.100.0/24 -o netdevice=<comp_iface_name> -o privileged=1 <your_network_name>

sudo docker run -d -it --privileged --name mount_dummy -v /sys/bus/pci/devices:/sys/bus/pci/devices -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev -v /mnt:/mnt sandiendndpdk:1.0.9
sudo docker run -d -it --privileged --volumes-from=mount_dummy --network <your_network_name> sandiendndpdk:1.0.9
```
