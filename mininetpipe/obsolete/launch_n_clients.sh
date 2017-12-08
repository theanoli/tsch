#! /bin/bash

if [ $# -ne 3 ]; then
	echo "Need to specify command to launch and number of clients!"
	echo "sh launch_n_clients.sh [command] [nclients] [nports]"	

	echo "Exiting..."
	exit
fi

cmd=$1
nclients=$2
nservers=$3

me=`basename "$0"`

echo "[$me] Using command $cmd"

portno=8000

for (( c=0; c<$nclients; c++ )); do
    if [ $(($c%100)) -eq 0 ]; then
        echo "[$me] Launching instance $c ..."
    fi
    $cmd -P $(($portno + $((c % nservers)))) &
done

echo "[$me] Brought up $(($c)) instances."
