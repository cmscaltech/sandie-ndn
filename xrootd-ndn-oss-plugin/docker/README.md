# NDN Dockerfile

This [Dockerfile](Dockerfile) creates Docker container for [XRootD NDN based OSS plugin](../README.md) and complementary applications based on CentOS 7.7 and the entire [NDN software stack](../../packaging/packaging.md).

## Build Docker image from Dockerfile

```bash

sudo docker build . --no-cache -t sandiendn:1.0.0  --network=host

```

## Run Docker container from image

```bash

sudo docker run --name=mysandiendn -it --cap-add=NET_ADMIN --net=host sandiendn:1.0.0

```
