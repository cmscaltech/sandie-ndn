#!/bin/sh
# args: <remote_host> <project_dir>
# eg: root@192.168.56.101 /root/sandie-ndn/xrootd-ndn-oss-plugin
buildDir=$2/build

echo "Copying xrdndn-producer files to remote"
scp -r ./CMakeLists.txt ./cmake ./src/xrdndn-producer $1:$2

echo "\n\nCompile xrdndn-producer on remote"
ssh $1 "zsh -l -c 'cd $buildDir && \
                            cmake ../ && \
                            make -j2 xrdndn-producer VERBOSE=1'"

echo "\n\nCopy xrdndn-producer from remote to local"
scp $1:$buildDir/xrdndn-producer ./build/xrdndn-producer

echo "\n\nAttaching to xrdndn-producer"
ssh -L9092:localhost:9092 $1 "gdbserver :9092 $buildDir/xrdndn-producer"