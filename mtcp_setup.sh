#! /bin/bash

mtcp=/proj/sequencer/mtcp
dpdk16=$mtcp/dpdk-16.04
dpdk=$mtcp/dpdk

if [ $# -lt 1 ]; then
	echo "Need a number for the IP address!"
fi

iface=`ifconfig | awk '!/10\.1\.1\..*/ {iface=$1}
			/10\.1\.1\..*/ {print iface}'`
ifconfig $iface down
echo "Brought down interface $iface; press any key "
echo "to continue..."

cd $dpdk16/tools
bash ./setup.sh
bash ./setup_iface_single_process.sh $1

cd $dpdk
ln -s $dpdk16/x86_64-native-linuxapp-gcc/lib lib
ln -s $dpdk16/x86_64-native-linuxapp-gcc/include include

cd $mtcp
./configure --with-dpdk-lib=$dpdk
sudo apt-get install -y libnuma-dev
make
