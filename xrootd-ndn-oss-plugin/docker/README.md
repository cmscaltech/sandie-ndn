# NDN Dockerfile

This Dockerfile (TODO: link) composes a Docker container with NDN software stack (TODO: link) alongised XRootD NDN OSS plugin, the producer and the consumer test application

## Build Docker image from Dockerfile

```bash

sudo docker build . --no-cache -t sandiendn:1.0.0  --network=host

```

## Run Docker container from image

```bash

sudo docker run --name=mysandiendn -it --cap-add=NET_ADMIN --net=host sandiendn:1.0.0

```

[TODO] How to use producer - consumer -> link to the other Readme file