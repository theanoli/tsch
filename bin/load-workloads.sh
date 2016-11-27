#! /bin/bash
#
# Script to copy YCSB/workloads/* over to Emulab clients' directories
#
# Usage: sh load-workloads.sh <emulab_user> <nclients>

if [ "$#" -ne 2 ]; then
	echo "Usage: $0 <emulab_user> <nclients>"
	exit 1
fi

EMULAB_USER=$1
NCLIENTS=$2

EXPID=sequencer.sequencer.emulab.net

for i in `seq 0 $(($NCLIENTS-1))`;
do
  (
  HOST=$EMULAB_USER@clients-$i.$EXPID
  scp ./YCSB/workloads/* $HOST:/usr/local/tsch/YCSB/workloads/
  ) &
done

wait
