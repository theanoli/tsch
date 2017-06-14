#! /bin/bash

# Must include path relative to the mininetpipe directory
while [ "$1" != "" ]; do
    case $1 in 
        -m | --mtcp )   shift
                        mtcp=$1
                        ;;
        -t | --tcp )    shift
                        tcp=$1
                        ;;
        * )             echo "Noooooope bad args"
                        exit 1
    esac
    shift
done

files=""
plot_cmd=""
counter=1

if [ "$mtcp" != "" ]; then
    cat $mtcp | tr -d " "
    expname=${mtcp%*.*}
    octave convert_raw_to_usec.oct $expname
    files=$files" $expname"
    plot_cmd=$plot_cmd" \"$expname.dat\" using ($counter):(\$2-\$1) title \"mTCP ($expname)\", "
    counter=$((counter + 1))
fi

if [ "$tcp" != "" ]; then
    cat $tcp | tr -d " "
    expname=${tcp%*.*}
    octave convert_raw_to_usec.oct $expname
    files=$files" $expname"
    plot_cmd=$plot_cmd"\"$expname.dat\" using ($counter):(\$2-\$1) title \"TCP ($expname)\""
    counter=$((counter + 1))
fi

echo "Plotting files$files..."
echo "Using plot command $plot_cmd"
gnuplot -e "files='$files'; plot_args='$plot_cmd'" plot_latency_multiprotocol.p
eog "results/plots_rtt.png"
