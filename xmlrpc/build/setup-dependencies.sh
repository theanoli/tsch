#! /bin/bash
#
# TODO(theano): All the things you need to install on the nodes,
# directories that need to get made, etc. 
# 
# Install:
# 	- libevent (memcached)
#

echo "Updating apt-get..."
sudo apt-get update -y > /dev/null
echo "Installing libevent..."
sudo apt-get install libevent-dev
