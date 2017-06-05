set autoscale
set datafile separator ","
set terminal png size 600,400 enhanced font "Helvetica,10"
set output "plot_latency.png"
unset log
unset label
set xtic auto
set ytic auto
set title "Client-server latency: mTCP"
set xlabel "Time (s)"
set ylabel "0.5 RTT (us)"
stats "np.out" using ($1 + $2/(10**9)):(($3*(10**6) + $4/(10**3)) - ($1*(10**6) + $2/(10**3))) name "plt"
plot "np.out" using ($1 + $2/(10**9) - plt_min_x):(($3*(10**6) + $4/(10**3)) - ($1*(10**6) + $2/(10**3))) title "Latency (us)"

