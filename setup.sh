#! /bin/bash
#
# Sets up an Emulab experimental environment.
#
# Loads a new experiment into Emulab if an NS
# file is given, else uses the existing NS file. Swaps in the
# experiment and loads the requisite binaries/dependencies onto
# the Emulab nodes.
#
# Not fully automated yet, as you need to put in a passphrase for
# xml-rpc.
#
# TODO(theano): 
#	- coordinate nclients/nservers here with nclients/nservers in ns
#	- permissions ugh
#
# Usage: sh setup.sh emulab_user nclients nservers [nsfile]
#

if [ "$#" -lt 3 ]; then
	echo "Usage: sh setup.sh emulab_user nclients nservers [nsfile]"
	exit 1 
fi

EMULAB_USER=$1
NCLIENTS=$2
NSERVERS=$3
NSFILE=$4

WRAPPER="python script_wrapper.py --server=boss.emulab.net" 
EXP=sequencer

cd xmlrpc

if [ $# -eq 4 ]; then
	echo "Modifying the experiment..."
	$WRAPPER modexp -w -e $EXP,$EXP "../$NSFILE"
	if [ $? -eq 0 ]
	then
		echo "Successfully modified experiment!"
	else
		echo "Failed to modify the experiment! Exiting..."
		exit 1
	fi
fi

echo "Attempting swap-in until we get nodes..."
ret=1
try=0
#while [ ! $ret -eq 0 ]; do 
for i in `seq 1 5`; do
	try=$(($try + 1))
	echo "Swap-in attempt $try"
	$WRAPPER swapexp -w -e $EXP,$EXP in
	ret=$?
done

echo "!!!Swap-in succeeded!!!"

