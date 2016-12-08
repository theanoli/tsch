#! /bin/bash
#
# Runs Python YCSB script on Emulab
#

if [ "$#" -ne 11 ]; then 
	echo "Usage $0 <config> <nthreads> <client_threads> <nservers> <nops> <output_dir> <me> <udp> <expected_tput> <conc_mult>"
	exit 1
fi

CONFIG=$1
NTHREADS=$2
CLIENT_THREADS=$3
NSERVERS=$4
NOPS=$5
OUTPUT_DIR=$6
ME=$7
EXP_LEN=$8
UDP=$9
EXPECTED_TPUT=${10}
CONC_MULT=${11}
 

cd /proj/sequencer/libmemcached/clients

mkdir -p $OUTPUT_DIR

# List the servers
EXPID=sequencer.sequencer.emulab.net

for i in `seq 0 $(($NSERVERS - 1))`; do SERVERS="$SERVERS,servers-$i.$EXPID:11211"; done
SERVERS=`echo $SERVERS | sed 's/^,//g'`


# Construct the command
cmd="./memaslap"

cmd="$cmd --servers=$SERVERS --threads=$CLIENT_THREADS --cfg_cmd=$CONFIG"

# Specify NOPS?
if [ "$NOPS" != "" ]; then 
	cmd="$cmd --execute_number=$NOPS"
fi

# Specify experiment length?
if [ "$EXP_LEN" != "" ]; then 
	cmd="$cmd --time=$EXP_LEN"
fi

# UDP? 
if [ "$UDP" = true ]; then 
	cmd="$cmd -U"
fi

# Specify desired throughput?
if [ "$EXPECTED_TPUT" != "" ]; then 
	cmd="$cmd --tps=$EXPECTED_TPUT"
fi

# Specify concurrency multiplier?
if [ "$CONC_MULT" != "" ]; then
	cmd="$cmd --concurrency=$(($CONC_MULT * $CLIENT_THREADS))"
fi


# Execute the command
pwd
echo -e "Client $i: Running command: \n\t$cmd"
echo

# $cmd 2> $OUTPUT_DIR/$i\_debug.data >> $OUTPUT_DIR/$i.data
$cmd

if [ $? -ne 0 ]
then
  echo "MEMASLAP FAILED..."
  exit 1
fi
