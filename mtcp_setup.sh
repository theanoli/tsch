#! /bin/bash

mtcp=/proj/sequencer/mtcp
dpdk16=$mtcp/dpdk-16.04
dpdk=$mtcp/dpdk

cd $dpdk16
bash tools/setup_iface_single_process.sh 4

cd $dpdk
ln -s $dpdk16/x86_64-native-linuxapp-gcc/lib lib
ln -s $dpdk16/x86_64-native-linuxapp-gcc/include include

cd $mtcp
./configure --with-dpdk-lib=$dpdk
sudo apt-get install libnuma-dev
make
