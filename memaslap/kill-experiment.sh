# Kills any running memcached instances
#

usage () {
	echo -e "Usage: bash kill-experiment.sh -e <emulab_user> -c <nclients> -s <nservers>"
}

if [ "$#" -ne 6 ]; then
	usage
	exit 1
fi 

while getopts ":he:c:s:" opt; do
case $opt in
	e) EMULAB_USER=$OPTARG;;
	s) NSERVERS=$OPTARG;;
	c) NCLIENTS=$OPTARG;;
	:) echo "Option -$OPTARG requires an argument." >&2
		exit 1;;
	\?) echo "Invalid option: -$OPTARG" >&2
		usage; exit 1;;
	h) usage; exit;;
esac
done

EMULAB_USER=$1
NCLIENTS=$2
NSERVERS=$3

EXPID=sequencer.sequencer.emulab.net
SSH="ssh -o StrictHostKeyChecking=no"

echo "Killing $NCLIENTS clients..."
for i in `seq 0 $(($NCLIENTS-1))`
do
	(echo "pkill -f 'memaslap'" | $SSH \
	$EMULAB_USER@clients-$i.$EXPID &> /dev/null) &
done

echo "Killing $NSERVERS servers..."
for i in `seq 0 $(($NSERVERS-1))`
do
	(echo "pkill memcached" | $SSH $EMULAB_USER@servers-$i.$EXPID &> /dev/null) &
done

wait
