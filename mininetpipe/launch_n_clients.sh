#! /bin/bash

if [ $# -ne 2 ]; then
	echo "Need to specify command to launch and number of clients!"
	echo "Exiting..."
	exit
fi

cmd=$1
nclients=$2

echo Using command $cmd

for (( c=1; c<=$nclients; c++ )); do
	echo "Launching instance $c ..."
	$cmd &
done
