#! /bin/bash
#
# Modeled after joshpfosi/dup_aware_scheduling git repo
#
# Sets up server & client node dependencies:
# 	- (what are they?)
#
# Usage: build-dependencies emulab_user nclients nservers
#

if [ "$#" -ne 2 ]; then 
	echo "Usage: $0 emulab_user nclients nservers"
	exit 1
fi


NCLIENTS=$2
NSERVERS=$3
EMULAB_USER=$1

EXPID=sequencer.sequencer
SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ServerAliveInterval=100"

for i in `seq $NSERVERS`
do
	(
		HOST=$EMULAB_USER@servers-$i.$EXPID.net
		echo "Setting up $HOST..."
		cat setup-dependencies | $SSH $HOST
	) &
done

for i in `seq $NCLIENTS`
do
	(
		HOST=$EMULAB_USER@clients-$i.$EXPID.net
		echo "Setting up $HOST..."
		cat setup-dependencies | $SSH $HOST
	) &
done

wait
