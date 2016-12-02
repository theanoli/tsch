# Kills any running memcached instances
#
# Usage ./kill-server <emulab_user> <nclients> <nservers>

if [ "$#" -ne 3 ]; then
	echo "Usage $0 <emulab-user> <nclients> <nservers>"
	exit 1
fi

EMULAB_USER=$1
NCLIENTS=$2
NSERVERS=$3

EXPID=sequencer.sequencer.emulab.net
SSH="ssh -o StrictHostKeyChecking=no"

echo "Killing $NCLIENTS clients..."
for i in `seq 0 $(($NCLIENTS-1))`
do
	(echo "pkill -f 'bin/ycsb'; pkill -f 'java'" | $SSH \
	$EMULAB_USER@clients-$i.$EXPID &> /dev/null) &
done

echo "Killing $NSERVERS servers..."
for i in `seq 0 $(($NSERVERS-1))`
do
	(echo "pkill memcached" | $SSH $EMULAB_USER@servers-$i.$EXPID &> /dev/null) &
done

wait
