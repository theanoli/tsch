#! /bin/bash
#
# Copies & builds YCSB on Emulab clients.
# 
# Usage: build-ycsb.sh emulab_user nclients
#

if [ "$#" -ne 2 ]; then 
	echo "Usage: $0 emulab_user nclients"
	exit 1
fi

EMULAB_USER=$1
NCLIENTS=$2

EXPID=sequencer.sequencer.emulab.net
SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ServerAliveInterval=100 -T"

for i in `seq 0 $(($NCLIENTS-1))`; 
do
	(
	HOST=$EMULAB_USER@clients-$i.$EXPID
	echo "Installing YCSB on $HOST..."
	scp ycsb.tar.gz $HOST:/usr/local/tsch
	$SSH $HOST 'bash -s' < setup-ycsb.sh 
	) &
done 

wait
