sudo bash turn_off_all_cores.sh

# NUMA node 1
sudo bash turn_on_some_cores.sh 4 4
sudo bash turn_on_some_cores.sh 8 8

# NUMA node 2
sudo bash turn_on_some_cores.sh 1 1
sudo bash turn_on_some_cores.sh 5 5
sudo bash turn_on_some_cores.sh 9 9

# NUMA node 3
sudo bash turn_on_some_cores.sh 2 2
sudo bash turn_on_some_cores.sh 6 6

