#! /bin/bash
#
# Sets up Emulab nodes: 
#	- installs dependencies and sets up directory
# 	- builds memcached
#	- builds YCSB
#
# Usage: build-emulab.sh <emulab_user> <nclients> <nservers>
#

# Check args
if [ "$#" -ne 3 ]; then
	echo "Usage: $0 <emulab_user> <nclients> <nservers>"
	exit 1
fi

EMULAB_USER=$1
NCLIENTS=$2
NSERVERS=$3

cd build

echo +~~~~~ BUILD DEPENDENCIES ~~~~~+
sh ./build-dependencies.sh $EMULAB_USER $NCLIENTS $NSERVERS

echo +~~~~~ BUILD MEMCACHED ~~~~~+ 
sh ./build-memcached.sh $EMULAB_USER 

echo +~~~~ BUILD YCSB ~~~~~+ 
sh ./build-ycsb.sh $EMULAB_USER $NCLIENTS
