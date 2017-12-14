# Dump CPUinfo to a particular file within an existing results directory

configdir=$1
configname=$2
configpath="$configdir/$configname"

echo "Dumping results to $configpath..."

cat /proc/cpuinfo >> $configpath; lscpu >> $configpath
