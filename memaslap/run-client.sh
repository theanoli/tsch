#! /bin/bash
#
# Runs Python YCSB script on Emulab
#

if [ "$#" -ne 10 ]; then 
	echo "Usage $0 <config> <nthreads> <client_threads> <nservers> <nops> <output_dir> <me> <udp> <expected_tput>"
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

 
# Set up the results directory if it doesn't exist
if [ ! -e $OUTPUT_DIR ]; then
	echo "Making directory $OUTPUT_DIR..."
	mkdir $OUTPUT_DIR
fi

cd /proj/sequencer/libmemcached/clients


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
