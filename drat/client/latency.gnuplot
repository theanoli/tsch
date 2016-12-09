set datafile separator ","
set terminal png size 900,400
set title "Latency"
set ylabel "RTT (ms)"
set xlabel "Time (ms)"
set xdata time
set ydata time
set timefmt "%s.%.9S"
set key left top
set grid
plot "doop2.txt" using 1:2 title 'rtt'


