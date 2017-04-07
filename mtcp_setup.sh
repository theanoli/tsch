#! /bin/bash

mtcp=/proj/sequencer/mtcp
dpdk16=$mtcp/dpdk-16.04
dpdk=$mtcp/dpdk

if [ $# -lt 1 ]; then
	echo "Need a number from 0-127 for the IP address!"
	exit
fi

# Clean up old builds
rm -f $dpdk/lib
rm -f $dpdk/include

cd $dpdk16/tools
bash /proj/sequencer/tsch/dpdk_setup.sh 

if [ $? -gt 0 ]; then
	echo "DPDK setup failed"
	exit 1
fi

bash ./setup_iface_single_process.sh $1

cd $dpdk
ln -s $dpdk16/x86_64-native-linuxapp-gcc/lib lib
ln -s $dpdk16/x86_64-native-linuxapp-gcc/include include

cd $mtcp
./configure --with-dpdk-lib=$dpdk
sudo apt-get install -y libnuma-dev
make
