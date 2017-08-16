## Synopsis

Stuff for transport protocol development and experiments.

## Installation

### mTCP
To install mTCP on a machine, run `sudo bash mtcp_setup.sh N`, where `N` is an integer that will become the last octet of that machine's IP address. Do not use the same number for two different machines at the same time!

This will configure/build/install both DPDK and mTCP.

For DPDK only, run `sudo bash dpdk-setup.sh`.

### Building the binaries
From the mininetpipe/ directory, type `make xyz`, where `xyz` is the protocol name you want to build. 

Available options as of now:
* mtcp (mTCP)
* udp (UDP)
* tcp (TCP)
* etcp (TCP using epoll to handle multiple clients)

## Running experiments

### Server
From the mininetpipe/ directory, run ./NPxyz. You will need sudo for mTCP. 

### Client: Latency
Latency is the default experiment type. Run `./NPxyz -H [hostname/IP address]`.

For additional options, the -h option will print help. 

The measurements will dump to a file that, by default, will go into the mininetpipe/results/ directory. The name will include the timestamp and relevant parameters you used to run the experiment.

### Client(s): Throughput
Use the -t flag to switch to throughput measurements. 

## Branches

The **master** branch is generally stable. The working branch is **devel**.
