set autoscale
set datafile separator ","
set terminal png size 900,400 enhanced font "Helvetica,10"
set output expname."_rtt.png"
unset log
unset label
set xtic auto
set ytic auto
set yrange [0:180]
set title "Client-server RTT: mTCP (".expname.")"
set xlabel "Time (s)"
set ylabel "1 RTT (us)"
stats expname.".out" using ($1 + $2/(10**9)):(($3*(10**6) + $4/(10**3)) - ($1*(10**6) + $2/(10**3))) name "plt"
plot expname.".out" using ($1 + $2/(10**9) - plt_min_x):(($3*(10**6) + $4/(10**3)) - ($1*(10**6) + $2/(10**3))) title "RTT (us)"

