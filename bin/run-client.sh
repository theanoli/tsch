#! /bin/bash
#
# Runs Python YCSB script on Emulab
#

if [ "$#" -ne 8 ]; then 
	echo "Usage $0 <port> <workload> <conns_per_server> <nthreads> <recordsize> <client_threads> <nservers> <nops>"
	exit 1
fi

PORT=$1
WORKLOAD=$2
CONNS_PER_SERVER=$3
NTHREADS=$4
RECORDSIZE=$5
CLIENT_THREADS=$6
NSERVERS=$7
NOPS=$8

EXPID=sequencer.sequencer.emulab.net

cd /usr/local/tsch/YCSB

for i in `seq 0 $(($NSERVERS - 1))`; do SERVERS="$SERVERS servers-$i.$EXPID:$PORT"; done
SERVERS=`echo $SERVERS | sed 's/^ *//g'`
SERVERS="memcached.hosts=$SERVERS"

MC_CONNS_PER_SERVER="memcached.connsPerServer=$CONNS_PER_SERVER"
MC_NTHREADS="memcached.numThreads=$NTHREADS"
MC_FIELDLENGTH="fieldlength=$RECORDSIZE"
MC_REQDIST="requestdistribution=uniform"
MC_RECCOUNT="recordcount=$NOPS"

./bin/ycsb load memcached -s -P workloads/$WORKLOAD \
	-p "$SERVERS" \
 	-p "$MC_CONNS_PER_SERVER" \
	-p "$MC_NTHREADS" \
	-p "$MC_FIELDLENGTH" \
	-p "$MC_REQDIST" \
	-p "$MC_RECCOUNT" \
	-threads $CLIENT_THREADS

./bin/ycsb run memcached -s -P workloads/$WORKLOAD \
	-p "$SERVERS" \
	-p "$MC_CONNS_PER_SERVER" \
	-p "$MC_NTHREADS" \
	-p "$MC_FIELDLENGTH" \
	-p "$MC_REQDIST" \
	-p "$MC_RECCOUNT" \
	-threads $CLIENT_THREADS

if [ $? -ne 0 ]
then
  echo "YCSB FAILED..."
  exit 1
fi
