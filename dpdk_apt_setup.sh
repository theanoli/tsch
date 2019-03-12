#! /bin/bash

iface=$1

sudo apt update

# Toolchain
sudo apt -y install g++-8 cmake

# General libs
sudo apt -y install libnuma-dev libgflags-dev libgtest-dev libboost-dev numactl
(cd /usr/src/gtest && sudo cmake . && sudo make && sudo mv libg* /usr/lib/)

# DPDK drivers
sudo apt -y install dpdk libdpdk-dev dpdk-igb-uio-dkms dpdk-doc dpdk-dev libdpdk-dev

. /usr/share/dpdk/dpdk-sdk-env.sh

sudo bash setup_hugepages.sh

sudo modprobe igb-uio
echo "Binding interface $iface to DPDK"
sudo ifconfig "$iface" down
sudo dpdk-devbind --bind=igb_uio "$iface"


