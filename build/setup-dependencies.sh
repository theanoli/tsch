#! /bin/bash
#
# TODO(theano): All the things you need to install on the nodes,
# directories that need to get made, etc. 
# 
# Install:
# 	- libevent (memcached)
#	- maven (YCSB)

echo "Updating apt-get..."
sudo apt-get update -y > /dev/null
echo "Installing libevent..."
sudo apt-get install -y libevent-dev > /dev/null
echo "Installing maven..."
sudo apt-get install -y maven > /dev/null
echo "Creating new working directory..."
sudo mkdir -m 777 -p /usr/local/tsch
