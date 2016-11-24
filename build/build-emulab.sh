#! /bin/bash
#
# Modeled after https://github.com/joshpfosi/dup_aware_scheduling
#
# Sets up Emulab nodes: 
# 	- build memcached
#
# Usage: build-emulab.sh emulab_user nclients nservers
#

# Check args
if [ "$#" -ne 2 ]; then
	echo "Usage: $0 emulab_user nclients nservers"
	exit 1
fi

EMULAB_USER=$1
NCLIENTS=$2
NSERVERS=$3

./build-dependencies.sh $EMULAB_USER $NCLIENTS $NSERVERS
./build-memcached.sh $EMULAB_USER $NSERVERS
./build-ycsb.sh $EMULAB_USER $NCLIENTS
