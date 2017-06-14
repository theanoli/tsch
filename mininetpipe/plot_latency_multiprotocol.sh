#! /bin/bash

# Must include path relative to the mininetpipe directory
files=()
counter=1
files=""
plot_cmd=""

for var in "$@"; do
    i=(($i + 1))
    expname=${var%*.*}
    octave convert_raw_to_usec.oct $expname
    files=$files" $expname"
    plot_cmd=$plot_cmd" \"$expname.dat\" using ($counter):(\$2-\$1) title \"$expname\", "
    counter=$((counter + 1))
done

echo "Plotting files$files..."
echo "Using plot command $plot_cmd"
gnuplot -e "files='$files'; plot_args='$plot_cmd'" plot_latency_multiprotocol.p
eog "results/plots_rtt.png"
