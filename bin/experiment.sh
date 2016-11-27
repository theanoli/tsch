#! /bin/bash
#
# Runs an experiment.
#
# Usage: sh experiment.sh <emulab_user> <WORKLOAD> <CONNS_PER_SERVER> <NTHREADS> <OUTPUT>
# <RECORDSIZE> <CLIENT_THREADS> <NCLIENTS> <NSERVERS> <NOPS>

if [ "$#" -ne 10 ]; then
	echo "Usage: $0 <emulab_user> <WORKLOAD> <CONNS_PER_SERVER> <NTHREADS> <OUTPUT> <RECORDSIZE> <CLIENT_THREADS> <NCLIENTS> <NSERVERS> <NOPS>"
	exit 1
fi

SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ServerAliveInterval=100 -T"

PORT=9456
EMULAB_USER=$1
WORKLOAD=$2
CONNS_PER_SERVER=$3
NTHREADS=$4
RECORDSIZE=$5
CLIENT_THREADS=$6
NCLIENTS=$7
NSERVERS=$8
NOPS=$9
OUTPUT=${10}

OUTPUT_DIR=$OUTPUT\_dir/

EXPID=sequencer.sequencer.emulab.net

MAX_CONNS=1024
REQ_CONNS=$(($NTHREADS * $CONNS_PER_SERVER * $CLIENT_THREADS * $NCLIENTS))

if [ $REQ_CONNS -ge $MAX_CONNS ]; then
	echo "Too many client connections ($REQ_CONNS). Memcached only supports $MAX_CONNS."
	exit 1
fi

mkdir -p $OUTPUT_DIR

for i in `seq 0 $(($NSERVERS-1))`
do
	HOST=$EMULAB_USER@servers-$i\.$EXPID
	echo "Booting $HOST..."
	cat run-server.sh | $SSH $HOST bash -s - $PORT $CONNS_PER_SERVER $NTHREADS &
done

# ------------------------------------------------------------------------------
# Run clients
# ------------------------------------------------------------------------------

echo $@ | tee -a $OUTPUT_DIR/$OUTPUT.data $OUTPUT_DIR/$OUTPUT-debug.data > /dev/null

for i in `seq 0 $(($NCLIENTS-1))`
do
	(
	echo "Running YCSB on client $i: workload=$WORKLOAD"
	cat run-client.sh | $SSH $EMULAB_USER@clients-$i\.$EXPID bash -s - \
	$PORT $WORKLOAD $CONNS_PER_SERVER $NTHREADS $RECORDSIZE $CLIENT_THREADS $NSERVERS \
	$NOPS 2> $OUTPUT_DIR/$OUTPUT-$i-debug.data | tee -a \
	$OUTPUT_DIR/$OUTPUT-$i-debug.data | grep Throughput | cut -d"," -f3 | \
	xargs >> $OUTPUT_DIR/$OUTPUT.data
 	) &
	pids[$i]=$!
done

for pid in ${pids[*]}; do wait $pid; done;

#
# Gather stats from each server and echo to $OUTPUT_server.data
#

# echo "Saving STATS from servers to $OUTPUT\_server.data..."
# for i in `seq 1 $NSERVERS`
# do
# 	read cmd_get cmd_set <<< `echo "stats" | nc server-$i.$EXPID $PORT | grep "cmd_get\|set" | cut -d" " -f3`
# 	cmd_get="${cmd_get/$'\r'/}" ; cmd_set="${cmd_set/$'\r'/}" ; total=`expr $cmd_get + $cmd_set`
# 	echo "$i $total $CONNS_PER_SERVER" >> $OUTPUT\_server.data
# 	# Use CONNS_PER_SERVER to color scatter plot by unaware/aware
# done

sh kill-experiment.sh $EMULAB_USER $NCLIENTS $NSERVERS
