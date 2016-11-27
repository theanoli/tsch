#! /bin/bash
#
# Untars & sets up YCSB on clients, memcached bindings only. Assumes: 
# 	- maven
# 

export DEBIAN_FRONTEND="noninteractive"
export LANG="C"

cd /usr/local/tsch
tar -xzf ycsb.tar.gz > /dev/null
rm -rf ycsb.tar.gz

sudo apt-get install openjdk-7-jdk
apt-cache search jdk
export JAVA_HOME=/usr/lib/jvm/java-7-openjdk
export PATH=$PATH:/usr/lib/jvm/java-7-openjdk-amd64/bin

cd YCSB
mvn -pl com.yahoo.ycsb:memcached-binding -am clean package > /dev/null
