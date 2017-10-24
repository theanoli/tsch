#! /bin/bash

cmd=$1
nclients=$2

echo Using command $cmd

for (( c=1; c<=$nclients; c++ )); do
	echo The "$c"th time...
	$cmd &
done
