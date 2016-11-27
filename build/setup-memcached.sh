#! /bin/bash
#
# Untar & build memcached. Assumes the following dependencies/preconditions:
# 	- memcached source lives in /usr/local
#	- libevent
#

export DEBIAN_FRONTEND "noninteractive"
export LANG "C"

cd /usr/local/tsch
tar -zxf memcached.tar.gz > /dev/null 
rm memcached.tar.gz

cd memcached
./configure --prefix=/usr/local/memcached > /dev/null 
make > /dev/null && sudo make install > /dev/null 
