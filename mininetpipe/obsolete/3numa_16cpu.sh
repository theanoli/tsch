sudo bash turn_off_all_cores.sh

# NUMA node 1
sudo bash turn_on_some_cores.sh 4 4
sudo bash turn_on_some_cores.sh 8 8
sudo bash turn_on_some_cores.sh 12 12
sudo bash turn_on_some_cores.sh 16 16
sudo bash turn_on_some_cores.sh 20 20

# NUMA node 2
sudo bash turn_on_some_cores.sh 1 1
sudo bash turn_on_some_cores.sh 5 5
sudo bash turn_on_some_cores.sh 9 9
sudo bash turn_on_some_cores.sh 13 13 
sudo bash turn_on_some_cores.sh 17 17 

# NUMA node 3
sudo bash turn_on_some_cores.sh 2 2
sudo bash turn_on_some_cores.sh 6 6
sudo bash turn_on_some_cores.sh 10 10
sudo bash turn_on_some_cores.sh 14 14
sudo bash turn_on_some_cores.sh 18 18

