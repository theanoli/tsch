#! /bin/bash

start=$1
end=$2

for x in $(seq $start $end); do
	sudo echo 0 > "/sys/devices/system/cpu/cpu$x/online"
done
