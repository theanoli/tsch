set autoscale
set datafile separator ","
unset log
unset label
set xtic auto
set ytic auto
set title "RTT over some interval"
set xlabel "Time"
set ylabel "RTT (us)"
plot "results/tmp/rtt_0.dat" using ($1*(10**6) + $2/(10**3)):(($3*(10**6) + $4/(10**3)) - ($1*(10**6) + $2/(10**3)))
#plot "results/tmp/rtt_0.dat" using 1:2
