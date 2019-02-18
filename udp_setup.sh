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

# Which CPUs should handle interrupts from which queues
#echo "Setting SMP affinity..."
#sudo bash set_smp_affinity.sh $iface $ncores_noht

# Which CPUs should handle packets from which queues; only
# do this when threads/processes are pinned to cores and each thread
# accepts traffic from a unique ip/port combo (threads don't share
# ip/ports)
if [[ $am_server -eq 1 ]]; then
    do_flowdir=0
    do_rfs=0
else
    do_flowdir=0
    do_rfs=0
fi
echo "Initializing RSS and RFS, FDir=$do_flowdir and RFS=$do_rfs..."
sudo bash initialize_rss_and_rfs.sh $iface $ncores_noht \
    $do_flowdir \
    $do_rfs

echo "Updating UDP flow hash rule..."
sudo ethtool -N $iface rx-flow-hash udp4 sdfn

