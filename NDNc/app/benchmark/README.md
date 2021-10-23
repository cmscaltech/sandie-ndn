# ndncft

```bash
# how to build apps
cd sandie-ndn && mkdir -p build

# -DCMAKE_BUILD_TYPE=Release/Debug/RelWithDebInfo/MinSizeRel
cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j16

# example how to run the file server application
./ndncft-server --gqlserver http://172.17.0.2:3030/

# example how to run the file client application
./ndncft-client --gqlserver http://172.17.0.2:3030/ --file /root/test.bin
```
