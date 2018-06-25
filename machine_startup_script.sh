# Turn off hyperthreading, turn off irqbalance, turn off collectl
# For each interface, set up RSS/RFS, affinitize rx/tx queue IRQs to cores,
# and use the SDFN hash for UDP
iface=$1

sudo pkill collectl
sudo service irqbalance stop
sudo bash turn_off_some_cores.sh 32 63  # turn off HT

sudo bash set_smp_affinity.sh $iface
sudo bash initialize_rss_and_rfs.sh $iface
sudo ethtool -N $iface rx-flow-hash udp4 sdfn

