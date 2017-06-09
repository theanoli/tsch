# Based on example here: http://psy.swansea.ac.uk/staff/carter/gnuplot/gnuplot_frequency.htm

set autoscale
set terminal png size 600,400 enhanced font "Helvetica,10"
set output expname."_intersend.png"
set key off
unset log
unset label

set title "RTT intersending times: mTCP (".expname.")"
set xlabel "Intersending time (us)"
set ylabel "Count"

set boxwidth (bin_width * 0.5) absolute 
set style fill solid 1.0 noborder

bin_number(x) = floor(x/bin_width)

rounded(x) = bin_width * ( bin_number(x) + 0.5 )

plot expname.".delta" using (rounded($1)):(1) smooth frequency with boxes

