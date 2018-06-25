#! /bin/bash

core=$1

sudo echo 0 > "/sys/devices/system/cpu/cpu$core/online"
