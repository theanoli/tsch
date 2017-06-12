#! /bin/bash

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

files=" "

if [ "$mtcp" != "" ]; then
    cat $mtcp | tr -d " "
    files=$files" $mtcp"
fi

if [ "$tcp" != "" ]; then
    cat $tcp | tr -d " "
    files=$files" $tcp"
fi

echo "Plotting files$files..."
gnuplot -e "files='$files'" plot_latency_multiprotocol.p
eog $files"_rtt.png"
