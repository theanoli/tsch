#! /bin/bash

for x in /sys/devices/system/cpu/cpu*/online; do
    sudo echo 1 > "$x"
done
