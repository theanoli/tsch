#! /bin/bash
#
# Runs an experiment.
#
# Usage: sh experiment.sh <emulab_user> <workload> <conns_per_server> <nthreads>
# <recordsize> <client_threads> <nclients> <nservers> <nops> <output>

if [ "$#" -ne 10 ]; then
	echo "Usage: sh $0 <emulab_user> <workload> <conns_per_server> <nthreads> <recordsize> <client_threads> <nclients> <nservers> <nops> <output>"
	exit 1
fi

SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ServerAliveInterval=100 -T"

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

OUTPUT_DIR=/proj/sequencer/tsch/results/$OUTPUT\_results/

EXPID=sequencer.sequencer.emulab.net

MAX_CONNS=1024
REQ_CONNS=$(($CLIENT_THREADS * $NCLIENTS))

if [ $REQ_CONNS -ge $MAX_CONNS ]; then
	echo "Too many client connections ($REQ_CONNS). Memcached only supports $MAX_CONNS."
	exit 1
fi

mkdir -p $OUTPUT_DIR

for i in `seq 0 $(($NSERVERS-1))`
do
	HOST=$EMULAB_USER@servers-$i\.$EXPID
	echo "Booting $HOST..."
	$SSH $HOST 'bash /proj/sequencer/tsch/bin/run-server.sh $CONNS_PER_SERVER $NTHREADS' &
	pids[$i]=$!
done

for pid in ${pids[*]}; do wait $pid; done;


# ------------------------------------------------------------------------------
# Run clients
# ------------------------------------------------------------------------------

# This will append, rather than overwrite, to save us from ourselves
echo $@ >> $OUTPUT_DIR/$OUTPUT.data

for i in `seq 0 $(($NCLIENTS-1))`
do
	HOST=$EMULAB_USER@clients-$i\.$EXPID
	(
	echo "Running YCSB on client $i: workload=$WORKLOAD"
	echo >> $OUTPUT_DIR/$OUTPUT.data
	echo "Client $i: workload=$WORKLOAD" >> $OUTPUT_DIR/$OUTPUT.data 
	$SSH $HOST 'bash /proj/sequencer/tsch/bin/run-client.sh $WORKLOAD $CONNS_PER_SERVER $NTHREADS \
	$RECORDSIZE $CLIENT_THREADS $NSERVERS \
	$NOPS $OUTPUT'
 	) &
	pids[$i]=$!
done

for pid in ${pids[*]}; do wait $pid; done;

# Delimit
echo >> $OUTPUT_DIR/$OUTPUT.data

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

bash kill-experiment.sh $EMULAB_USER $NCLIENTS $NSERVERS
