#! /bin/bash
#
# All Emulab nodes share a filesystem, so build this on the first
# server only
#
# This assumes you have memcache tarball in the build directory.
#
# Usage: build-memcached.sh <emulab_user>

if [ "$#" -ne 1 ]; then
	echo "Usage: $0 <emulab_user>"
	exit 1
fi

EMULAB_USER=$1
EXPID=sequencer.sequencer.emulab.net

HOST=$EMULAB_USER@servers-0.$EXPID
SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ServerAliveInterval=100 -T"

echo "Building memcached on $HOST..."
scp memcached.tar.gz $HOST:/usr/local/tsch
$SSH $HOST 'bash -s' < setup-memcached.sh
