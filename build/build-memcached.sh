#! /bin/bash
#
# All Emulab nodes share a filesystem, so build this on the first
# server only
#
# This assumes you have memcache tarball in the build directory.
#
# Usage: build-memcached.sh emulab_user

if [ "$#" -ne 1 ]; then
	echo "Usage: $0 emulab_user"
	exit 1
fi

EMULAB_USER=$1
EXPID=sequencer.sequencer
SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ServerAliveInterval=100"

HOST=$EMULAB_USER@servers-0.$EXPID.net

echo "Building memcached on $HOST..."
scp memcached $HOST:/usr/local/
cat setup-memcached.sh | $SSH $HOST

