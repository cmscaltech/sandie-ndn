# NDNc file transfer applications

```bash
# How to build 
cd sandie-ndn && mkdir -p build

# -DCMAKE_BUILD_TYPE=Release/Debug/RelWithDebInfo/MinSizeRel
cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j16
```

## The server application

```bash
# How to run the file server application
./ndncft-server --gqlserver http://172.17.0.2:3030/
```

## The client application

```bash
# How to list one file
./ndncft-client --gqlserver http://172.17.0.2:3030/ --list /root/test.bin

# How to list one directory
./ndncft-client --gqlserver http://172.17.0.2:3030/ --list /root/

# How to list one directory recursively
./ndncft-client --gqlserver http://172.17.0.2:3030/ --list /root/ --recursive

# How to list one or more files or directories recursively
./ndncft-client --gqlserver http://172.17.0.2:3030/ -l /root/ /user/file /user/folders/ -r

# How to copy one or more files or directories
./ndncft-client --gqlserver http://172.17.0.2:3030/ --copy /root/ /user/file /user/folders/

# How to copy one or more files or directories recursively
./ndncft-client --gqlserver http://172.17.0.2:3030/ -c /root/ /user/file /user/folders/ -r
```
