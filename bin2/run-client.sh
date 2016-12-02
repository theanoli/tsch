#! /bin/bash
#
# Runs Python YCSB script on Emulab
#

if [ "$#" -ne 8 ]; then 
	echo "Usage $0 <workload> <conns_per_server> <nthreads> <recordsize> <client_threads> <nservers> <nops> <output_dir>"
	exit 1
fi

WORKLOAD=$1
CONNS_PER_SERVER=$2
NTHREADS=$3
RECORDSIZE=$4
CLIENT_THREADS=$5
NSERVERS=$6
NOPS=$7
OUTPUT_DIR=$8

EXPID=sequencer.sequencer.emulab.net
NRECS=100000
         
 
#---------- Set up the results directory if it doesn't exist ----------#
echo "Making directory $OUTPUT_DIR..."
mkdir -p $OUTPUT_DIR


cd /proj/sequencer/YCSB

for i in `seq 0 $(($NSERVERS - 1))`; do SERVERS="$SERVERS,servers-$i.$EXPID"; done
SERVERS=`echo $SERVERS | sed 's/^,//g'`
SERVERS="memcached.hosts=$SERVERS"

MC_CONNS_PER_SERVER="memcached.connsPerServer=$CONNS_PER_SERVER"
MC_NTHREADS="memcached.numThreads=$NTHREADS"
MC_FIELDLENGTH="fieldlength=$RECORDSIZE"
MC_REQDIST="requestdistribution=uniform"
MC_RECCOUNT="recordcount=$NRECS"
MC_OPSCOUNT="operationcount=$NOPS"

./bin/ycsb load memcached -s -P workloads/$WORKLOAD \
	-p "$SERVERS" \
 	-p "$MC_CONNS_PER_SERVER" \
	-p "$MC_NTHREADS" \
	-p "$MC_FIELDLENGTH" \
	-p "$MC_REQDIST" \
	-p "$MC_RECCOUNT" \
	-p "$MC_OPSCOUNT" \
	-threads $CLIENT_THREADS \
	2> $OUTPUT_DIR/$i-debug.data \
	>> $OUTPUT_DIR/$i.data

./bin/ycsb run memcached -s -P workloads/$WORKLOAD \
	-p "$SERVERS" \
	-p "$MC_CONNS_PER_SERVER" \
	-p "$MC_NTHREADS" \
	-p "$MC_FIELDLENGTH" \
	-p "$MC_REQDIST" \
	-p "$MC_RECCOUNT" \
	-p "$MC_OPSCOUNT" \
	-threads $CLIENT_THREADS
	2> $OUTPUT_DIR/$i-debug.data \
	>> $OUTPUT_DIR/$i.data

if [ $? -ne 0 ]
then
  echo "YCSB FAILED..."
  exit 1
fi
