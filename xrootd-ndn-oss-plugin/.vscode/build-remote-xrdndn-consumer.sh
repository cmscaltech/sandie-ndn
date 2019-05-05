#!/bin/sh
# args: <remote_host> <project_dir>
# eg: root@192.168.56.101 /root/sandie-ndn/xrootd-ndn-oss-plugin
buildDir=$2/build

echo "Copying xrdndn-consumer files to remote"
scp -r ./CMakeLists.txt ./cmake ./src/xrdndn-consumer $1:$2

echo "\n\nCompile xrdndn-consumer on remote"
ssh $1 "zsh -l -c 'cd $buildDir && \
                            cmake ../ && \
                            make -j2 xrdndn-consumer VERBOSE=1'"

echo "\n\nCopy xrdndn-consumer from remote to local"
scp $1:$buildDir/xrdndn-consumer ./build/xrdndn-consumer

echo "\n\nAttaching to xrdndn-consumer"
ssh -L9091:localhost:9091 $1 "gdbserver :9091 $buildDir/xrdndn-consumer --input-file /root/path/to/testfiles/for/xrootd/ndn/test_file"