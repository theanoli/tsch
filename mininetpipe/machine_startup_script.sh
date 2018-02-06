# Turn off hyperthreading, turn off irqbalance, turn off collectl
# For each interface, set up RSS/RFS, affinitize rx/tx queue IRQs to cores,
# and use the SDFN hash for UDP
cd /proj/sequencer/tsch/mininetpipe

sudo pkill collectl
sudo service irqbalance stop
sudo bash turn_off_some_cores.sh 32 63

for var in "$@"; do
    sudo bash set_smp_affinity.sh $var 
    sudo bash initialize_rss_and_rfs.sh $var
    sudo ethtool -N $var rx-flow-hash udp4 sdfn
done
