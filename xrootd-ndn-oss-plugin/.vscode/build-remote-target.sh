#!/bin/sh
remote_host=root@192.168.0.69
projdirname=xrootd-ndn-oss-plugin

echo $PWD

#copy new source files remote
echo "copy new files on remote"
scp -r ../$projdirname $remote_host:.

#compile new source files remote
echo "compile new files on remote"
ssh $remote_host "source /opt/rh/devtoolset-7/enable && zsh -l -c 'cd $projdirname && make -f Makefile $1'"

#copy new binery from remote server
echo "copy newly build $1 from remote server"
scp $remote_host:./$projdirname/bin/$1 ./bin/