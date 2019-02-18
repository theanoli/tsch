# Turn off hyperthreading, turn off irqbalance, turn off collectl
# For each interface, set up RSS/RFS, affinitize rx/tx queue IRQs to cores,
# and use the SDFN hash for UDP
iface=$1
am_server=$2
ncores=`lscpu | awk '$1 == "CPU(s):" {print $2}'`
ncores_noht=$(($ncores/2))

sudo pkill collectl
sudo service irqbalance stop

echo "Turning off cores $ncores_noht through $(($ncores-1))..."
sudo bash turn_off_some_cores.sh $ncores_noht $(($ncores-1))   # turn off HT

# Reduce number of queues to match number of cores
echo "Setting nqueues == ncores..."
sudo ethtool -L $iface combined $ncores_noht
