# NDN Dockerfile

This [Dockerfile](Dockerfile) creates Docker container for NDN-DPDK based [consumer and producer applications](../README.md) based on CentOS 8.

## Build Docker image from Dockerfile

```bash
sudo docker build . -t sandiendndpdk_docker_img  --network=host

```

## Run Docker container from image

```bash

sudo docker run --ulimit core=-1 -d -it --privileged -v /sys/bus/pci/devices:/sys/bus/pci/devices -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev -v /mnt:/mnt --network=host sandiendndpdk_docker_img

```

## Run Docker containers using SR-IOV specification

```bash

sudo docker pull rdma/container_tools_installer
sudo docker pull rdma/sriov-plugin
sudo docker run -v /usr/bin:/tmp --net=host rdma/container_tools_installer
sudo docker run -d --rm -v /run/docker/plugins:/run/docker/plugins -v /etc/docker:/etc/docker -v /var/run:/var/run --net=host --privileged rdma/sriov-plugin
sudo docker network create -d sriov --subnet=100.10.100.0/24 -o netdevice=<comp_iface_name> -o privileged=1 <your_network_name>

sudo docker run --ulimit core=-1 -d -it --privileged -v /sys/bus/pci/devices:/sys/bus/pci/devices -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev -v /mnt:/mnt --network=<your_network_name> sandiendndpdk_docker_img
```
