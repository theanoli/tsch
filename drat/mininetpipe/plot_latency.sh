#! /bin/bash

# Delete padding spaces
file="results/`ls ./results | sort | head -1`"
cat $file | tr -d " "

# Remove file extension from filename
expname=${file%*.*}

echo $expname
gnuplot -e "expname='$expname'" plot_latency.p
eog $expname.png
