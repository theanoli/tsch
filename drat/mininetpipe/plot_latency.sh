#! /bin/bash

# Delete padding spaces
cat np.out | tr -d " "

gnuplot plot_latency.p
eog plot_latency.png
