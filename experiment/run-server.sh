# Runs a Memcached server on Emulab
#
# Usage: ./run-server <conns_per_server> <nthreads>
#

if [ "$#" -ne 2 ]; then
	echo "Usage $0 <conns_per_server> <nthreads>"
	exit 1
fi

CONNS_PER_SERVER=$1
NTHREADS=$2

# TODO(theano) what about conns per server?
cd /proj/sequencer/memcached
./memcached -t $NTHREADS
