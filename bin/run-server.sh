# Runs a Memcached server on Emulab
#
# Usage: ./run-server <port> <conns_per_server> <nthreads>
#

if [ "$#" -ne 3 ]; then
	echo "Usage $0 <port> <conns_per_server> <nthreads>"
	exit 1
fi

PORT=$1
CONNS_PER_SERVER=$2
NTHREADS=$3

# TODO(theano) what about conns per server?
cd /usr/local/tsch/memcached
./memcached -p $PORT -t $NTHREADS
