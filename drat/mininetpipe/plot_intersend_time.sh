#! /bin/bash

# Delete padding spaces
file="results/`ls ./results | sort | tail -1`"
cat $file | tr -d " "

# Remove file extension from filename
expname=${file%*.*}

echo $expname
gnuplot -e "expname='$expname'" plot_intersend_time.p
eog $expname"_intersend.png"


