#! /usr/bin/python

import os
import re
import sys
import argparse
import subprocess

# nclient processes TOTAL, list of client IPs, command to run (for each process?)

parser = argparse.ArgumentParser(description='Run a single server process'
						'and multiple client processes.')
parser.add_argument('serv_cmd',
			help='Command to run for each server process.')
parser.add_argument('cli_cmd', 
			help='Command to run for each client process.')
parser.add_argument('nodes', help='List of client machine IP addresses or names.')
parser.add_argument('nprocs', 
			type=int,
			help=('Total number of client processes to launch '
				'(will be spread evenly over the client machines.'))

args = parser.parse_args()

cli_cmd = args.cli_cmd
serv_cmd = args.serv_cmd
nodes = [".".join([x, "drat.sequencer.emulab.net"]) for x in args.nodes.split(",")]
nprocs = args.nprocs

procs_per_client = nprocs/len(nodes)
leftover = nprocs % len(nodes)

wdir = "/proj/sequencer/tsch/mininetpipe"
launcher = os.path.join(wdir, "launch_n_clients.sh")

# Launch the server-side program and then wait a second
subprocess.Popen(
    "cd %s; %s" % (wdir, serv_cmd),
    shell=True)

for ip in nodes:
    n = procs_per_client

    if leftover > 0: 
        n += 1
        leftover -= 1 

    subprocess.Popen(
        "ssh theano@%s 'cd %s; bash %s \"%s\" %d'" % (ip, wdir, launcher, cli_cmd, n),
        shell=True)
    
 
