# Turn off hyperthreading, turn off irqbalance, turn off collectl
sudo pkill collectl
sudo service irqbalance stop
sudo bash turn_off_some_cores.sh 32 63
