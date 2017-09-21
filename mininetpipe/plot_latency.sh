#! /bin/bash

# One and only argument should be the filename relative to the
# results/ directory.

# Delete padding spaces
if [ -z "$1" ]; then 
    file="results/`ls ./results -t | grep "\.out" | head -1`"
else
    file="results/$1"
fi

tr -d " " < $file > tmp.txt
mv tmp.txt $file
echo $file

# Remove file extension from filename
expname=${file%*.*}

echo "Converting raw data to microseconds..."
octave convert_raw_to_usec.oct $expname

echo $expname
gnuplot -e "expname='$expname'" plot_latency.p
eog $expname"_rtt.png"
