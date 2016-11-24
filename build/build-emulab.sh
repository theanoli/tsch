#! /bin/bash
#
# Sets up Emulab nodes: 
#	- installs dependencies and sets up directory
# 	- builds memcached
#	- builds YCSB
#
# Usage: build-emulab.sh emulab_user nclients nservers
#

# Check args
if [ "$#" -ne 3 ]; then
	echo "Usage: $0 emulab_user nclients nservers"
	exit 1
fi

EMULAB_USER=$1
NCLIENTS=$2
NSERVERS=$3

sh ./build-dependencies.sh $EMULAB_USER $NCLIENTS $NSERVERS
sh ./build-memcached.sh $EMULAB_USER 
sh ./build-ycsb.sh $EMULAB_USER $NCLIENTS
