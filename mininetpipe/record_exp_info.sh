# To be run at the beginning of a *set* of experiments. 
# Not intended to be robust. Assumes 1 server, 2 clients.
# Creates a new directory to put results and the output of this file.
# Records:
#   * Timestamp (in filename)
#   * Dump of CPUinfo (stable) 

dirname=$(date +"%Y-%m-%d-%H-%M")
configdir="/proj/sequencer/tsch/mininetpipe/results/$dirname"
configpath="$configdir/config"
cpuinfo="cat /proc/cpuinfo >> $configpath; lscpu >> $configpath"
linebreak="--------------------------------------------------------------------------------"
echo "Dumping results to $dirname..."
mkdir $configdir
touch $configpath

echo $linebreak >> $configpath
echo "BEGIN SERVER INFO" | tee -a $configpath
eval "$cpuinfo"
echo "END SERVER INFO" | tee -a $configpath
echo $linebreak >> $configpath

echo $linebreak >> $configpath
echo "BEGIN CLIENT-0 INFO" | tee -a $configpath
ssh client-0.drat.sequencer.emulab.net bash -c "'$cpuinfo'"
echo "END CLIENT-0 INFO" | tee -a $configpath
echo $linebreak >> $configpath

echo $linebreak >> $configpath
echo "BEGIN CLIENT-1 INFO" | tee -a $configpath
ssh client-1.drat.sequencer.emulab.net bash -c "'$cpuinfo'"
echo "END CLIENT-1 INFO" | tee -a $configpath
echo $linebreak >> $configpath

echo $linebreak >> $configpath
echo "BEGIN CLIENT-2 INFO" | tee -a $configpath
ssh client-2.drat.sequencer.emulab.net bash -c "'$cpuinfo'"
echo "END CLIENT-2 INFO" | tee -a $configpath
echo $linebreak >> $configpath
