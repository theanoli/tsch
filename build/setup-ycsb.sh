#! /bin/bash
#
# Untars & sets up YCSB on clients, memcached bindings only. Assumes: 
# 	- maven
# 

export DEBIAN_FRONTEND "noninteractive"
export LANG "C"

cd /usr/local/tsch
tar -xzf ycsb.tar.gz > /dev/null
rm -rf ycsb.tar.gz

cd YCSB
mvn -pl com.yahoo.ycsb:memcached-binding -am clean package > /dev/null
