set autoscale
set terminal png size 900,400 enhanced font "Helvetica,10"
set output expname."_rtt.png"
unset log
unset label
set xtic auto
set ytic auto
#set yrange [0:180]
set datafile separator ","
set title "Client-server RTT ".expname
set xlabel "Time (us)"
set ylabel "1 RTT (us)"

stats expname.".dat" using 1:($2 - $1) name "plt"
plot expname.".dat" using ($1 - plt_min_x):($2 - $1) title "RTT (us)"
