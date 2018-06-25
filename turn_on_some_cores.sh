#! /bin/bash

start=$1
end=$1

for x in $(seq $start $end); do
	sudo echo 1 > "/sys/devices/system/cpu/cpu$x/online"
done
