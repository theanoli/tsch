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

EXPID=sequencer.sequencer.emulab

for i in `seq 0 $(($NCLIENTS-1))`; 
do
	(
	HOST=$EMULAB_USER@client-$i.$EXPID.net
	echo "Installing YCSB on $HOST..."
	scp ycsb.tar.gz $HOST:/usr/local/
	cat setup-ycsb.sh | $SSH $HOST
	) &
done 

wait
