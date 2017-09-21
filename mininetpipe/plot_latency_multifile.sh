#! /bin/bash

# Must include path relative to the mininetpipe directory
script_dir=`pwd`

files=""

counter=1
plot_cmd=""

cd $1
data_dir=`pwd`

for var in *.out; do
    expname=${var%*.*}
    octave $script_dir/convert_raw_to_usec.oct $expname
    files=$files" $data_dir$expname"
    plot_cmd=$plot_cmd" \"$data_dir/$expname.dat\" using ($counter):(\$2-\$1) title \"$expname\","
    counter=$((counter + 1))
done

echo "Plotting files$files..."
echo "Using plot command $plot_cmd"
gnuplot -e "outfile='$data_dir/plots_rtt.png'; files='$files'; plot_args='$plot_cmd'" $script_dir/plot_latency_multiprotocol.p
eog "$data_dir/plots_rtt.png"

