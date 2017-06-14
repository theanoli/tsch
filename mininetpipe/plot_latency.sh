#! /bin/bash

# Delete padding spaces
file="results/`ls ./results | grep "\.out" | sort | tail -1`"
cat $file | tr -d " "
echo $file

# Remove file extension from filename
expname=${file%*.*}

octave convert_raw_to_usec.oct $expname

echo $expname
gnuplot -e "expname='$expname'" plot_latency.p
eog $expname"_rtt.png"
