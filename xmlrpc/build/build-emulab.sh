#! /bin/bash
#
# Modeled after https://github.com/joshpfosi/dup_aware_scheduling
#
# Sets up Emulab nodes: 
# 	- build memcached
#
# Usage: build-emulab nclients nservers
#

# Check args
if [ "$#" -ne 2 ]; then
	echo "Usage: $0 nclients nservers"
	exit 1
fi


NCLIENTS=$1
NSERVERS=$2

./build-dependencies $NCLIENTS $NSERVERS
./build-memcached $NSERVERS
