#! /bin/bash

# Delete padding spaces
file="results/`ls ./results | sort | head -1`"
cat $file | tr -d " "
expname=${file%*.*}

echo $expname
gnuplot -e "expname='$fname'" plot_latency.p
eog $expname.png
