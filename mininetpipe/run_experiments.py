#! /usr/bin/python

import os
import re
import sys
import argparse
import subprocess
import time
import shlex

def run_experiment(cli_cmd, serv_cmd, nodes, nprocs, nservers):
    # All as in usage, after converting to appropriate type
    procs_per_client = nprocs/len(nodes)
    leftover = nprocs % len(nodes)

    wdir = "/proj/sequencer/tsch/mininetpipe"
    launcher = os.path.join(wdir, "launch_n_clients.sh")

    # Clean up any potential zombies on the client side(s)
    print "****Killing zombie processes on client...****"
    for node in nodes: 
        subprocess.Popen(
            shlex.split("ssh %s 'sudo pkill \"NP[a-z]*\"'" % node))
    time.sleep(3)

    # Launch the server-side program and then wait a half-second
    print "****Launching server program:...****"
    os.chdir(wdir)
    command = shlex.split("%s -c %d" % (serv_cmd, nprocs))

    servers = []
    for i in range(nservers): 
        cmd = command + ["-P %d" % (8000 + i)]
        print "Sending command to server:"
        print cmd
        servers.append(subprocess.Popen(cmd))

    time.sleep(1)

    print "****Getting ready to launch clients...****"

    # Launch the client-side programs
    for i, node in enumerate(nodes):
        n = procs_per_client

        if leftover > 0: 
            n += 1
            leftover -= 1 

        command = shlex.split("ssh %s 'cd %s; bash %s \"%s\" %d %d'" % (
                node, wdir, launcher, cli_cmd, n, nservers))
        print "Sending command to client:\n"
        print command
        subprocess.Popen(command)

    for server in servers:
        server.wait()
    

# if __name__ == "__main__":
#     parser = argparse.ArgumentParser(description='Run a single server process'
#             'and multiple client processes.')
#     parser.add_argument('ntrials',
#             type=int,
#             help='Number of times to repeat experiment')
#     parser.add_argument('serv_cmd',
#             help='Command to run for each server process.')
#     parser.add_argument('cli_cmd',
#             help='Command to run for each client process.')
#     parser.add_argument('nodes', 
#             help='List of client machine IP addresses or names.')
#     parser.add_argument('nprocs',
#             type=int,
#             help=('Total number of client processes to launch '
#                 '(will be spread evenly over the client machines).'))
# 
#     args = parser.parse_args()
# 
#     ntrials = args.ntrials
#     cli_cmd_arg = args.cli_cmd
#     serv_cmd_arg = args.serv_cmd
#     nodes_arg = [".".join([x, "drat.sequencer.emulab.net"]) for x in args.nodes.split(",")]
#     nprocs_arg = args.nprocs
#     port_range_arg = args.port_range
# 
#     for trial in range(ntrials):
#         print ""
#         print "***Running trial %d!***" % (trial + 1)
#         run_experiment(cli_cmd_arg, serv_cmd_arg, nodes_arg, nprocs_arg)
#         print "***Completed trial %d!***" % (trial + 1)
#         print ""


