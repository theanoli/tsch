#! /bin/usr/bash

#---------------------------------------
# MTCP parameters
# 
#	-N number of cores
# 	-s hostname
#	-f configfile
#	-c concurrency
#	-o output_dir
#	-u sleeptime (usec)
#	-l do latency measurements
#	-t do throughput measurements
#	-d experiment duration
#
# --------------------------------------

#------- PARAMETERS -------#
# Options + arguments
NCORES=1
SERVIP="10.0.0.5"
CONF="epwget.conf"
CONCURRENCY=10000
OUTDIR=""  # output directory
SLEEPTIME=10000  # usec
DURATION=15

# Flags
LATENCY=0
THROUGHPUT=0


#------- Arg parsing -------#
# Set server to 10.0.0.5 by convention
while getopts ":N:s:f:c:o:u:d:lt" opt; do
	case $opt in
	N)
		NCORES=$OPTARG
		;;
	s)
		SERVIP=$OPTARG
		;;
	f)
		CONF=$OPTARG
		;;
	o)
		OUTDIR=$OPTARG
		;;
	u)
		SLEEPTIME=$OPTARG
		;;
	d)
		DURATION=$OPTARG
		;;
	l)
		LATENCY=1
		;;
	t)
		THROUGHPUT=1
		;;
	esac
done

if [ "$OUTDIR" = "" ]; then 
	# Set to timestamp
	OUTDIR="$(date +"%Y%m%d_%H%M%S")"
fi
datadir="results/$OUTDIR"

command="sudo ./client -N $NCORES -s $SERVIP -f $CONF -o $OUTDIR -u $SLEEPTIME -d $DURATION"
parameters="NCORES=$NCORES SLEEPTIME(us)=$SLEEPTIME EXPDURATION=$DURATION"

if [ "$LATENCY" -eq 1 ]; then
	command=$command" -l"
	parameters=$parameters" +L"
fi

if [ "$THROUGHPUT" -eq 1 ]; then
	command=$command" -t"
	parameters=$parameters" +T"
fi

if [ ! -e "$datadir" ]; then 
	mkdir $datadir
fi

echo "Dumping parameters to file..."
echo $parameters > "$datadir/parameters.txt"

echo "Running experiment with parameters $parameters"
eval $command
 
echo "Generating plot..."
gnuplot -e "datadir='$datadir/'; plot_title='$parameters'" plot_rtt.p

eog $datadir/rtt_0.png &

