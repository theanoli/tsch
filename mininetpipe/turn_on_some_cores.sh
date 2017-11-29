#! /bin/bash

for x in {32..63}; do
	sudo echo 1 > "/sys/devices/system/cpu/cpu$x/online"
done
