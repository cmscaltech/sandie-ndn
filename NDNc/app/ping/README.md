# ndncping

```bash
# how to build apps
cd sandie-ndn && mkdir build
cd build && cmake ../
make

# example how to run ping server application
./ndncping-server --gqlserver http://172.17.0.2:3030 --mtu 9000 --name /example/P --payload 6144

# example how to run ping client application
./ndncping-client --gqlserver http://172.17.0.2:3030 --mtu 9000 --name /example/P
```
