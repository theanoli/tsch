sudo bash turn_off_some_cores.sh 32 63
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 32cores_nopin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 32 --collect_stats --results_filebase 32cores_nopin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80

sudo bash turn_off_some_cores.sh 16 31
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 16cores_nopin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 16 --collect_stats --results_filebase 16cores_nopin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80

sudo bash turn_off_some_cores.sh 8 15 
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 8cores_nopin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 8 --collect_stats --results_filebase 8cores_nopin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80

# 4 cores
sudo bash turn_off_some_cores.sh 4 7 
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 4cores_nopin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 4 --collect_stats --results_filebase 4cores_nopin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80

# 2 cores
sudo bash turn_off_some_cores.sh 2 3 
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 2cores_nopin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 2 --collect_stats --results_filebase 2cores_nopin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80

# 1 core
sudo bash turn_off_some_cores.sh 1 1 
sh record_cpuinfo_to_config.sh results/2017-12-20-16-44 1cores_nopin.conf
python run_experiments.py "./NPtcp -t" client-1,client-0 --results_dir 2017-12-20-16-44 --ntrials 20 --nservers 1 --collect_stats --results_filebase 1cores_nopin --ncli_min 2048 --wait_multiplier 0.01 --expduration 80

