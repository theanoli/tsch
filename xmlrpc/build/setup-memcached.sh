#! /bin/bash
#
# Untar & build memcached. Assumes the following dependencies/preconditions:
# 	- memcached source lives in /usr/local
#	- libevent
#

cd /usr/local
tar -zxf memcached.tar.gz > /dev/null
cd memcached
./configure --prefix=/usr/local/memcached > /dev/null
make && sudo make install > /dev/null
