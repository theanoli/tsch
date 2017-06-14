set autoscale
set macros
set terminal png size 600,900 enhanced font "Helvetica,10"
set output "results/plots_rtt.png"
unset log
unset label
set xtics auto
set yrange [*:*]
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

# In case we later want to do point plots
# stats expname.".out" using ($1 + $2/(10**9)):(($3*(10**6) + $4/(10**3)) - ($1*(10**6) + $2/(10**3))) name "plt"
# plot expname.".out" using ($1 + $2/(10**9) - plt_min_x):(($3*(10**6) + $4/(10**3)) - ($1*(10**6) + $2/(10**3))) title "RTT (us)"
