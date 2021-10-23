# ndncping

```bash
# how to build apps
cd sandie-ndn && mkdir -p build
# -DCMAKE_BUILD_TYPE=Release/Debug/RelWithDebInfo/MinSizeRel
cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j16

# example how to run ping server application
./ndncping-server --gqlserver http://172.17.0.2:3030 --mtu 9000 --name /example/P --payload 6144

# example how to run ping client application
./ndncping-client --gqlserver http://172.17.0.2:3030 --mtu 9000 --name /example/P
```
