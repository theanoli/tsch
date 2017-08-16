#! /bin/bash

# Delete padding spaces
file="results/`ls ./results -t | grep "\.out" | head -1`"
cat $file | tr -d " "
echo $file

# Remove file extension from filename
expname=${file%*.*}

# Generate a file with the inter-sending times
echo "Converting raw data to microseconds..."
octave convert_raw_to_usec.oct $expname

tail -n +6 $expname.dat > $expname.tmp && mv $expname.tmp $expname.dat

echo "Generating deltas..."
octave generate_deltas.oct $expname

# Generate a plot from the inter-send times
echo $expname

echo "Gnuplotting..."
gnuplot -e "expname='$expname'; bin_width=5" plot_intersend_time.p
eog $expname"_intersend.png"

