#! /bin/bash
#
# Runs Python YCSB script on Emulab
#

if [ "$#" -ne 9 ]; then 
	echo "Usage $0 <config> <nthreads> <client_threads> <nservers> <nops> <output_dir> <me>"
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


EXPID=sequencer.sequencer.emulab.net

 
# Set up the results directory if it doesn't exist
echo "Making directory $OUTPUT_DIR..."
mkdir -p $OUTPUT_DIR

echo "CDing into the YCSB directory..."
cd /proj/sequencer/libmemcached/clients/memaslap

cmd="./memaslap"

# List the servers
for i in `seq 0 $(($NSERVERS - 1))`; do SERVERS="$SERVERS,servers-$i.$EXPID:11211"; done
SERVERS=`echo $SERVERS | sed 's/^,//g'`

cmd="$cmd $SERVERS --threads=$CLIENT_THREADS --execute_number=$NOPS --time=$EXP_LEN --cfg_cmd=$CONFIG "

if [ "$UDP" = true ]; then 
	cmd="$cmd -U"
fi

if [ "$EXPECTED_TPUT" != "" ]; then 
	cmd="$cmd --tps=$EXPECTED_TPUT"
fi

echo $cmd


if [ $? -ne 0 ]
then
  echo "MEMASLAP FAILED..."
  exit 1
fi
