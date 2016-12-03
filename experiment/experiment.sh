#! /bin/bash
#
# Runs an experiment. Run on the local machine; the remote machines must be
# up-to-date on changes to run-client.sh and run-server.sh.
#
# Usage: sh experiment.sh <emulab_user> <workload> <conns_per_server> <nthreads>
# <recordsize> <client_threads> <nclients> <nservers> <nops> <output>

SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ServerAliveInterval=100 -T"

# u = user
# w = workload
# p = conns per server
# t = nthreads for memcached
# z = recordsize
# n = threads per client
# c = nclients
# s = nservers
# o = nops
# f = output dir/filename

usage () {
	echo "Usage: sh $0 -u <emulab_user> -w <workload> -t <nthreads> -n <client_threads> -c <nclients> -s <nservers> -o <nops> [-z <recordsize> -p <conns_per_server> -f <output>]"
}

CONNS_PER_SERVER=1024
RECORDSIZE=64
OUTPUT=output

while getopts ":hu:w:p:t:z:n:c:s:o:f:" opt; do 
case $opt in
	u) EMULAB_USER=$OPTARG;;
	w) WORKLOAD=$OPTARG;; 
	p) CONNS_PER_SERVER=$OPTARG;;
	t) NTHREADS=$OPTARG;;
	z) RECORDSIZE=$OPTARG;;
	n) CLIENT_THREADS=$OPTARG;;
	c) NCLIENTS=$OPTARG;;
	s) NSERVERS=$OPTARG;;
	o) NOPS=$OPTARG;;
	f) OUTPUT=$OPTARG;;
	:) echo "Option -$OPTARG requires an argument." >&2
		exit 1;;
	\?) echo "Invalid option: -$OPTARG" >&2
		usage; exit 1;;
	h) usage; exit;; 
esac
done

echo Args: 
echo "\tEMULAB_USER: $EMULAB_USER"
echo "\tWORKLOAD: $WORKLOAD"
echo "\tCONNS_PER_SERVER: $CONNS_PER_SERVER"
echo "\tNTHREADS: $NTHREADS"
echo "\tRECORDSIZE: $RECORDSIZE"
echo "\tCLIENT_THREADS: $CLIENT_THREADS"
echo "\tNSERVERS: $NSERVERS"
echo "\tNCLIENTS: $NCLIENTS"
echo "\tNOPS: $NOPS"
echo "\tOUTPUT: $OUTPUT"
echo

NOW=`date +%s`
OUTPUT_DIR=/proj/sequencer/tsch/results/$OUTPUT\_$NOW\_results/
BIN_DIR="/proj/sequencer/tsch/experiment"

EXPID=sequencer.sequencer.emulab.net

MAX_CONNS=1024
REQ_CONNS=$(($CLIENT_THREADS * $NCLIENTS))

if [ $REQ_CONNS -ge $MAX_CONNS ]; then
	echo "Too many client connections ($REQ_CONNS). Memcached only supports $MAX_CONNS."
	exit 1
fi


for i in `seq 0 $(($NSERVERS-1))`
do
	HOST=$EMULAB_USER@servers-$i\.$EXPID
	echo "Booting $HOST..."
	$SSH $HOST "bash \"$BIN_DIR/run-server.sh\" \"$CONNS_PER_SERVER\" \"$NTHREADS\"" &
done


# ------------------------------------------------------------------------------
# Run clients
# ------------------------------------------------------------------------------

# This will append, rather than overwrite, to save us from ourselves
for i in `seq 0 $(($NCLIENTS-1))`
do
	HOST=$EMULAB_USER@clients-$i\.$EXPID
	( echo "Running YCSB on client $i: workload=$WORKLOAD"
	$SSH $HOST "bash \"$BIN_DIR\"/run-client.sh \"$WORKLOAD\" \"$CONNS_PER_SERVER\" \"$NTHREADS\" \"$RECORDSIZE\" \"$CLIENT_THREADS\" \"$NSERVERS\" \"$NOPS\" \"$OUTPUT_DIR\" \"$i\""
 	> /dev/null ) &
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

bash kill-experiment.sh $EMULAB_USER $NCLIENTS $NSERVERS
