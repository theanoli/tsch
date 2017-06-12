#! /bin/bash

# Delete padding spaces
file="results/`ls ./results | sort | tail -1`"
cat $file | tr -d " "

# Remove file extension from filename
expname=${file%*.*}

# Generate a file with the inter-sending times
octave convert_raw_to_usec.oct $expname
octave generate_deltas.oct $expname

# Generate a plot from the inter-send times
gnuplot -e "expname='$expname'; bin_width=5" plot_intersend_time.p
eog $expname"_intersend.png"


