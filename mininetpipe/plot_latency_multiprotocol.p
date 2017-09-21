set autoscale
set macros
set datafile separator ','
set terminal png size 900,600 enhanced font "Helvetica,10"
set output outfile
unset log
unset label
set xtics auto
set yrange [*:200]
set title "Client-server RTT"
set xlabel "Time (s)"
set ylabel "RTT (us)"

set style fill solid 0.25 border -1
set style boxplot nooutliers pointtype 7
set style data boxplot
set boxwidth 0.5
set pointsize 0.5

set border 2
plot @plot_args

