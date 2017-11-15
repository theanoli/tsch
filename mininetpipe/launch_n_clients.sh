#! /bin/bash

if [ $# -ne 2 ]; then
	echo "Need to specify command to launch and number of clients!"
	echo "sh launch_n_clients.sh [command] [nclients]"	

	echo "Exiting..."
	exit
fi

cmd=$1
nclients=$2

me=`basename "$0"`

echo "[$me] Using command $cmd"

for (( c=1; c<=$nclients; c++ )); do
    if [ $(($c%100)) -eq 0 ]; then
        echo "[$me] Launching instance $c ..."
    fi
    $cmd &
done

echo "[$me] Brought up $(($c - 1)) instances."
