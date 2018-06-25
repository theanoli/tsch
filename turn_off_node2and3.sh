#! /bin/bash

for x in $(seq 0 63); do
    if [ $(($x % 4)) -gt 1 ]; then
        echo "Turning off $x"
	    sudo echo 0 > "/sys/devices/system/cpu/cpu$x/online"
    fi 
done
