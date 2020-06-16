#!/bin/sh

export HDFS_DATANODE_USER=root
export JAVA_HOME=
export HADOOP_NAMENODE_OPTS="-Xms8453M -Xmx8453M -XX:+UseConcMarkSweepGC -XX:ParallelGCThreads=8 -XX:+UseCMSInitiatingOccupancyOnly -XX:CMSInitiatingOccupancyFraction=70 -XX:NewSize=1024M -XX:MaxNewSize=1024M"
export HADOOP_HEAPSIZE="8453"
