# Turn off hyperthreading, turn off irqbalance, turn off collectl
sudo pkill collectl
sudo service irqbalance stop
sudo bash turn_off_some_cores.sh 32 63

if [ $@ -gt 0 ]; do
    sudo bash set_smp_affinity.sh $1
done 
