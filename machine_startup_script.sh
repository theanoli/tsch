# Turn off hyperthreading, turn off irqbalance, turn off collectl
# For each interface, set up RSS/RFS, affinitize rx/tx queue IRQs to cores,
# and use the SDFN hash for UDP
iface=$1
ncores=`lscpu | awk '$1 == "CPU(s):" {print $2}'`
ncores_noht=$(($ncores/2))

sudo pkill collectl
sudo service irqbalance stop

echo "Turning off cores $ncores_noht through $(($ncores-1))..."
sudo bash turn_off_some_cores.sh $ncores_noht $(($ncores-1))   # turn off HT

echo "Setting SMP affinity..."
sudo bash set_smp_affinity.sh $iface $ncores_noht

echo "Initializing RSS and RFS..."
sudo bash initialize_rss_and_rfs.sh $iface $ncores_noht

echo "Updating UDP flow hash rule..."
sudo ethtool -N $iface rx-flow-hash udp4 sdfn

