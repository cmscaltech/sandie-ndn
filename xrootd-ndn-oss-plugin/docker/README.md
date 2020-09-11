# NDN Dockerfile

This [Dockerfile](Dockerfile) creates Docker containers for [XRootD NDN based OSS plugin](../README.md) and complementary applications based on CentOS 7.x and the [NDN software stack](https://github.com/cmscaltech/sandie-ndn-repo).

## Build Docker image from Dockerfile

```console
user@cms:~$ sudo docker build . --no-cache -t sandiendn:1.0.0  --network=host
```

## Run Docker container from image

```console
user@cms:~$ docker run -d -it --name=sandiendn --cap-add=NET_ADMIN --net=host sandiendn:1.0.0
```

## Run simple test
```console
user@cms:~$ docker exec -it sandiendn bash
# Check: https://github.com/named-data/NFD/blob/master/docs/INSTALL.rst for config
user@cms:~$ cp /etc/ndn/nfd.conf.sample /etc/ndn/nfd.conf
user@cms:~$ xrdndn-producer
user@cms:~$ xrdndn-consumer --input-file <filepath>
```
