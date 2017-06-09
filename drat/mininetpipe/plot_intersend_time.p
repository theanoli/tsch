delta_v(x) = ( vD = x - old_v, old_v = x, vD)
old_v = NaN
binwidth=5
bin(x,width)=width*floor(x/width)

set autoscale
set datafile separator ","
set terminal png size 900,400 enhanced font "Helvetica,10"
set output expname."_intersend.png"
unset log
unset label
set xtic auto
set ytic auto
set title "RTT intersending times: mTCP (".expname.")"
set xlabel "Intersending time (us)"
set ylabel "Count"
plot expname.".out" using (bin(delta_v(($1*(10**6) + $2/(10**3))), binwidth)):(1.0) smooth freq with boxes
