set autoscale
set datafile separator ","
set terminal png size 600,400 enhanced font "Helvetica,10"
set output datadir."rtt_0.png"
unset log
unset label
set xtic auto
set ytic auto
set title "RTT over some interval"
set xlabel "Time (s)"
set ylabel "RTT (us)"
stats datadir."/rtt_0.dat" using ($1 + $2/(10**9)):(($3*(10**6) + $4/(10**3)) - ($1*(10**6) + $2/(10**3))) name "plt"
plot datadir."/rtt_0.dat" using ($1 + $2/(10**9) - plt_min_x):(($3*(10**6) + $4/(10**3)) - ($1*(10**6) + $2/(10**3))) title plot_title
