#! /bin/bash

for x in {1..4}; do
	sudo echo 1 > "/sys/devices/system/cpu/cpu$x/online"
done
