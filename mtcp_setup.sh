#! /bin/bash

mtcp=/proj/sequencer/mtcp
dpdk16=$mtcp/dpdk-16.11
dpdk=$mtcp/dpdk

if [ $# -lt 2 ]; then
	echo "Need iface name and a number from 0-127 for the IP address!"
	exit
fi

# Clean up old builds
rm -f $dpdk/lib
rm -f $dpdk/include

bash /proj/sequencer/tsch/dpdk_setup.sh $1

if [ $? -gt 0 ]; then
	echo "DPDK setup failed"
	exit 1
fi

cd $dpdk16/tools
bash ./setup_iface_single_process.sh $2

cd $dpdk
ln -s $dpdk16/x86_64-native-linuxapp-gcc/lib lib
ln -s $dpdk16/x86_64-native-linuxapp-gcc/include include

cd $mtcp
./configure --with-dpdk-lib=$dpdk
sudo apt-get install -y libnuma-dev
make
