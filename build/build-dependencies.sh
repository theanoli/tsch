#! /bin/bash
#
# Sets up server & client node dependencies:
# 	- (what are they?)
#
# Usage: build-dependencies <emulab_user> <nclients> <nservers>
#

if [ "$#" -ne 3 ]; then 
	echo "Usage: $0 <emulab_user> <nclients> <nservers>"
	exit 1
fi

export DEBIAN_FRONTEND=noninteractive 

NCLIENTS=$2
NSERVERS=$3
EMULAB_USER=$1

EXPID=sequencer.sequencer.emulab.net
SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ServerAliveInterval=100 -T"

for i in `seq 0 $(($NSERVERS-1))`
do
	(
		HOST=$EMULAB_USER@servers-$i.$EXPID
		echo "Setting up $HOST..."
		$SSH $HOST 'bash -s' < setup-dependencies.sh
	) &
done

for i in `seq 0 $(($NCLIENTS-1))`
do
	(
		HOST=$EMULAB_USER@clients-$i.$EXPID
		echo "Setting up $HOST..."
		$SSH $HOST 'bash -s' < setup-dependencies.sh 
	) &
done

wait
