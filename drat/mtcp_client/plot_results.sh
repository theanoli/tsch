gnuplot -e "datadir='$1/'; plot_title='`cat $1/parameters.txt`'" plot_rtt.p
eog $1/rtt_0.png
