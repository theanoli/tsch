sudo bash turn_off_all_cores.sh

# NUMA node 1
sudo bash turn_on_some_cores.sh 4 4
sudo bash turn_on_some_cores.sh 8 8
sudo bash turn_on_some_cores.sh 12 12

# NUMA node 2
sudo bash turn_on_some_cores.sh 1 1
sudo bash turn_on_some_cores.sh 5 5
sudo bash turn_on_some_cores.sh 9 9
sudo bash turn_on_some_cores.sh 13 13 
