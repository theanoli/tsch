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
	echo "Usage: sh $0 -u <emulab_user> -c <config> -t <nthreads> -n <client_threads> -c <nclients> -s <nservers> -o <nops> [-l <exp_len> -f <output> -u -p <expected_tput>]"
}

OUTPUT=output
EXP_LEN=2m
UDP=false

while getopts ":hu:l:c:p:t:n:c:s:o:f:" opt; do 
case $opt in
	u) EMULAB_USER=$OPTARG;;
	c) CONFIG=$OPTARG;; 
	t) NTHREADS=$OPTARG;; # memcached threads
	n) CLIENT_THREADS=$OPTARG;;
	c) NCLIENTS=$OPTARG;;
	s) NSERVERS=$OPTARG;;
	o) NOPS=$OPTARG;;
	p) EXPECTED_TPUT=$OPTARG;;
	f) OUTPUT=$OPTARG;;
	l) EXP_LEN=$OPTARG;;
	u) UDP=true;;
	:) echo "Option -$OPTARG requires an argument." >&2
		exit 1;;
	\?) echo "Invalid option: -$OPTARG" >&2
		usage; exit 1;;
	h) usage; exit;; 
esac
done

echo Args: 
echo -e "\tEMULAB_USER: $EMULAB_USER"
echo -e "\tCONFIG: $CONFIG"
echo -e "\tNTHREADS: $NTHREADS"
echo -e "\tCLIENT_THREADS: $CLIENT_THREADS"
echo -e "\tNSERVERS: $NSERVERS"
echo -e "\tNCLIENTS: $NCLIENTS"
echo -e "\tNOPS: $NOPS"
echo -e "\tOUTPUT: $OUTPUT"
echo -e "\tEXP_LEN: $EXP_LEN"
echo -e "\tUDP: $UDP"
echo -e "\tEXPECTED_TPUT: $EXPECTED_TPUT"
echo

NOW=`date +%s`
OUTPUT_DIR=/proj/sequencer/tsch/results/$OUTPUT\_$NOW\_results
BIN_DIR="/proj/sequencer/tsch/memaslap"
CONFIG_DIR="$BIN_DIR/configs"

EXPID=sequencer.sequencer.emulab.net


for i in `seq 0 $(($NSERVERS-1))`
do
	HOST=$EMULAB_USER@servers-$i\.$EXPID
	echo "Loading memcached on $HOST..."
	$SSH $HOST "bash \"$BIN_DIR/run-server.sh\" \"$NTHREADS\"" &
done


# ------------------------------------------------------------------------------
# Run clients
# ------------------------------------------------------------------------------

# This will append, rather than overwrite, to save us from ourselves
for i in `seq 0 $(($NCLIENTS-1))`
do
	HOST=$EMULAB_USER@clients-$i\.$EXPID
	( echo "Running memaslap on client $i: config=$CONFIG"
	$SSH $HOST "bash \"$BIN_DIR\"/run-client.sh \"$CONFIG_DIR/$CONFIG\" \"$NTHREADS\" \"$CLIENT_THREADS\" \"$NSERVERS\" \"$NOPS\" \"$OUTPUT_DIR\" \"$i\" \"$EXP_LEN\" \"$UDP\" \"$EXPECTED_TPUT\""
 	> /dev/null ) &
	pids[$i]=$!
done

for pid in ${pids[*]}; do wait $pid; done;

bash kill-experiment.sh $EMULAB_USER $NCLIENTS $NSERVERS
