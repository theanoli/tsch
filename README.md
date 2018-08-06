## Synopsis

Stuff for transport protocol development and experiments.

## Setup

### Before setting everything up... 
Copy and paste the list of machines from the "List View" tab in your experiment's Emulab portal. 

### mTCP
To install mTCP on all machines, run `python parse_machine_list.py [your emulab ID] --setup_mtcp`. 

### DPDK
To set up DPDK on all machines (without mTCP), run `python parse_machine_list.py [your Emulab ID] --setup_DPDK`.

### Linux TCP/UDP
To disable irqbalance and collectl, and to enable RSS/RFS, run `python parse_machine_list.py [your Emulab ID] --setup_rss`. This will steer flows bound for particular ports to particular CPUs (e.g., packets destined for port 8000 will go to receive queue 0, which will be pinned to core 0), as well as killing collectl and irqbalance and turning off hyperthreading on 32-core machines. You will need to modify this script if you are on a machine with a different core count. 

### Building the binaries
From the mininetpipe/ directory, type `make xyz`, where `xyz` is the protocol name you want to build. 

Available options as of now:
* mtcp (mTCP)
* udp (UDP)
* tcp (TCP)

## Running experiments
The script `run_experiments.py` will launch the specified number of server and client processes when launched from the server machine. A basic call for throughput would be: 

`python run_experiments.py "./NPudp -t" client-0,client-1,client-2`

Other options (number of server processes, etc.) can be found by calling the program with `--help`. 

### Lower-level experiments
Launching client and server processes manually may be helpful for debugging. Note that the usage (can get by running `./NPxyz -h`) is probably incomplete.

#### Server
From the mininetpipe/ directory, run `./NPxyz` for latency, `./NPxyz -t` for throughput. You will need sudo for mTCP. 

#### Client(s)
Run `./NPxyz -H [hostname/IP address]`. Use the -t flag to switch to throughput measurements. 

## Branches

The **master** branch is generally stable. The working branch is **devel**.
