#!/bin/sh
remote_host=root@192.168.56.101
projdirname=xrootd-ndn-oss-plugin
buildir=$projdirname/build

#copy new source files remote
echo "copy new files on remote"
scp -r ../$projdirname/src ../$projdirname/cmake ../$projdirname/rpms ../$projdirname/CMakeLists.txt  $remote_host:./$projdirname

#compile new source files remote
echo "compile new files on remote"
ssh $remote_host "source /opt/rh/devtoolset-7/enable && \
                zsh -l -c 'cd $buildir && \
                            rm -rf CMake\* && \
                            cmake ../  && \
                            make $1 VERBOSE=1'"

#copy new binery from remote server
echo "copy newly build $1 from remote server"
scp $remote_host:./$buildir/$1 ./build/