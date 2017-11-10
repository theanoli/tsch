#! /usr/bin/python

import os
import re
import sys
import argparse
import subprocess
import time


def run_experiment(cli_cmd, serv_cmd, nodes, nprocs):
    # All as in usage, after converting to appropriate type
    procs_per_client = nprocs/len(nodes)
    leftover = nprocs % len(nodes)

    wdir = "/proj/sequencer/tsch/mininetpipe"
    launcher = os.path.join(wdir, "launch_n_clients.sh")

    # Clean up any potential zombies on the client side(s)
    for ip in nodes: 
        subprocess.Popen(
            "ssh theano@%s 'pkill \"NP[a-z]*\" -U theano'" % ip,
            shell=True)
    time.sleep(3)

    # Launch the server-side program and then wait a half-second
    server = subprocess.Popen(
        "cd %s; %s -c %d" % (wdir, serv_cmd, nprocs),
        shell=True)

    time.sleep(0.5)

    # Launch the client-side programs
    for ip in nodes:
        n = procs_per_client

        if leftover > 0: 
            n += 1
            leftover -= 1 

        subprocess.Popen(
            "ssh theano@%s 'cd %s; bash %s \"%s\" %d'" % (ip, wdir, launcher, cli_cmd, n),
            shell=True)

    server.wait()
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run a single server process'
            'and multiple client processes.')
    parser.add_argument('ntrials',
            type=int,
            help='Number of times to repeat experiment')
    parser.add_argument('serv_cmd',
            help='Command to run for each server process.')
    parser.add_argument('cli_cmd',
            help='Command to run for each client process.')
    parser.add_argument('nodes', 
            help='List of client machine IP addresses or names.')
    parser.add_argument('nprocs',
            type=int,
            help=('Total number of client processes to launch '
                '(will be spread evenly over the client machines).'))                        

    args = parser.parse_args()

    ntrials = args.ntrials
    cli_cmd_arg = args.cli_cmd
    serv_cmd_arg = args.serv_cmd
    nodes_arg = [".".join([x, "drat.sequencer.emulab.net"]) for x in args.nodes.split(",")]
    nprocs_arg = args.nprocs

    for trial in range(ntrials):
        print ""
        print "***Running trial %d!***" % (trial + 1)
        run_experiment(cli_cmd_arg, serv_cmd_arg, nodes_arg, nprocs_arg)
        print "***Completed trial %d!***" % (trial + 1)
        print ""


