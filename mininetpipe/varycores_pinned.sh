# Set up RSS -- note there are only 63 queues!
#sudo ethtool -K eth5 ntuple on

# for x in `seq 0 63`; do
#     if [ $x -gt 9 ]; then
#         port=80$x
#     else
#         port=800$x
#     fi
#     sudo ethtool -N eth5 flow-type tcp4 dst-port $port action $x
# done
# 
# # Set up RFS
# echo 32768 | sudo tee /proc/sys/net/core/rps_sock_flow_entries
# 
# for x in `seq 0 62`; do
#     echo 512 | sudo tee /sys/class/net/eth5/queues/rx-$x/rps_flow_cnt
# done
# sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 64cores_pin.conf
#python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 64 --collect_stats --results_filebase 64cores_pin --ncli_min 2048 --wait_multiplier 0.02 --expduration 80 --pin_procs

# 32 cores
for x in `seq 0 31`; do
    echo 1024 | sudo tee /sys/class/net/eth5/queues/rx-$x/rps_flow_cnt
done
sudo bash turn_off_some_cores.sh 32 63
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 32cores_pin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 32 --collect_stats --results_filebase 32cores_pin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80 --pin_procs

# 16 cores
for x in `seq 0 15`; do
    echo 2048 | sudo tee /sys/class/net/eth5/queues/rx-$x/rps_flow_cnt
done
sudo bash turn_off_some_cores.sh 16 31
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 16cores_pin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 16 --collect_stats --results_filebase 16cores_pin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80 --pin_procs

# 8 cores
for x in `seq 0 15`; do
    echo 4096 | sudo tee /sys/class/net/eth5/queues/rx-$x/rps_flow_cnt
done
sudo bash turn_off_some_cores.sh 8 15 
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 8cores_pin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 8 --collect_stats --results_filebase 8cores_pin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80 --pin_procs

# 4 cores
for x in `seq 0 15`; do
    echo 8192 | sudo tee /sys/class/net/eth5/queues/rx-$x/rps_flow_cnt
done
sudo bash turn_off_some_cores.sh 4 7 
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 4cores_pin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 4 --collect_stats --results_filebase 4cores_pin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80 --pin_procs

# 2 cores
for x in `seq 0 15`; do
    echo 16384 | sudo tee /sys/class/net/eth5/queues/rx-$x/rps_flow_cnt
done
sudo bash turn_off_some_cores.sh 2 3 
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 2cores_pin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 2 --collect_stats --results_filebase 2cores_pin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80 --pin_procs

# 1 core
for x in `seq 0 15`; do
    echo 32768 | sudo tee /sys/class/net/eth5/queues/rx-$x/rps_flow_cnt
done
sudo bash turn_off_some_cores.sh 1 1 
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 1cores_pin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 1 --collect_stats --results_filebase 1cores_pin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80 --pin_procs



