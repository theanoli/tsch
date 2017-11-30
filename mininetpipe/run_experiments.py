#! /usr/bin/python

import os
import re
import sys

import argparse
import math
import shlex
import subprocess
import time

class ExperimentSet(object):
    def __init__(self, args):
        self.printlabel = "[" + os.path.basename(__file__) + "]"

        self.basecmd = args.basecmd
        self.server_ip = args.server_ip
        self.nodes = [".".join([x, "drat.sequencer.emulab.net"])
                if "client" in x else x
                for x in args.nodes.split(",")]
        self.ntrials = args.ntrials
        self.ncli_min = args.ncli_min
        self.ncli_max = args.ncli_max
        self.nservers = args.nservers
        self.start_port = args.start_port
        self.wdir = args.wdir
        self.results_dir = args.results_dir
        self.results_file = args.results_file

        # If we have more server processes than client procs, override the number of
        # client procs to ensure each server proc gets at least one client proc
        if self.ncli_min < self.nservers:
            self.ncli_min = self.nservers

        if self.ncli_max < self.ncli_min:
            self.ncli_max = self.ncli_min

    def printer(self, msg): 
        print "%s %s" % (self.printlabel, msg)
    
    def run_experiments(self):
        # Launch the entire set of experiments for this experiment set
        n = self.ncli_min

        while n <= self.ncli_max:
            for trial in range(self.ntrials):
                self.printer("Running trial %d for %d clients per node" % 
                        (trial + 1, n))
                experiment = Experiment(self, n)
                self.printer("Completed trial %d!" % (trial + 1))
            n *= 2


class Experiment(object):
    def __init__(self, experiment_set, nprocs):
        self.experiment_set = experiment_set
        self.nprocs_per_client = nprocs / len(self.nodes)
        self.total_clientprocs = nprocs
        self.online_wait = math.ceil(self.total_clientprocs * 0.001)

        self.run_experiment()

    def __getattr__(self, attr):
        return getattr(self.experiment_set, attr)

    def kill_zombie_processes(self):
        # Clean up any potential zombies on the client side(s)
        self.printer("Killing zombie processes on client(s).")
        killers = []
        for node in self.nodes: 
            killers.append(subprocess.Popen(
                shlex.split("ssh %s 'sudo pkill \"NP[a-z]*\"'" % node)))
        for killer in killers:
            killer.wait()

    def launch_servers(self):
        # Launch the server-side program and then wait a bit 
        self.printer("Launching servers...")
        
        os.chdir(self.wdir)

        servers = []
        for i in range(self.nservers): 
            serv_cmd = (self.basecmd +
                    " -c %d" % (self.total_clientprocs / self.nservers) + 
                    " -P %d" % (self.start_port + i) +
                    " -w %d" % (self.online_wait))
            if self.results_dir:
                serv_cmd += " -d %s" % self.results_dir
            if self.results_file:
                serv_cmd += " -o %s" % self.results_file
            cmd = shlex.split(serv_cmd)
            self.printer("Launching server process %d: %s" % (i, serv_cmd))
            servers.append(subprocess.Popen(cmd))

        time.sleep(0.5)
        return servers

    def launch_clients(self):
        # Launch the client-side programs
        self.printer("Launching clients...")

        for node in self.nodes:
            # Create a giant list of all the commands to be executed on a
            # single node; this avoids opening a ton of SSH sessions
            nodecmds = ""
            for n in range(self.nprocs_per_client):
                if n % 100 == 0:
                    self.printer("Launching client process %d on %s" % 
                            (n, node))
                cmd = (self.basecmd + 
                        " -H %s" % self.server_ip +
                        " -P %d" % (self.start_port + (n % self.nservers)))
                nodecmds += (cmd + " &\n")

            # Commands can be really long; dump to file
            fname = "cmdfile_%s.sh" % node
            f = open(fname, "w")
            f.write(nodecmds)
            subprocess.Popen(shlex.split("ssh %s 'cd %s; bash -s' < %s" % 
                (node, self.wdir, fname)))
            f.close()
        
    def run_experiment(self):
        self.kill_zombie_processes()

        servers = self.launch_servers()
        self.launch_clients()

        for server in servers:
            server.wait()

    
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run one or more server processes '
            'and multiple client processes.')
    parser.add_argument('basecmd',
            help=('Command to run for each process. Will be augmented '
            'with port number and total number of clients for server, '
            'server IP and port number for clients.'))
    parser.add_argument('nodes', 
            help='List of client machine IP addresses or names.')
    parser.add_argument('--server_ip',
            help=('IP address of the server. Default 10.1.1.3.'),
            default="10.1.1.3") 
    parser.add_argument('--ntrials',
            type=int,
            help='Number of times to repeat experiment. Default 1.',
            default=1)
    parser.add_argument('--ncli_min',
            type=int,
            help=('Min number of clients to run. Default 1.'),
            default=1)
    parser.add_argument('--ncli_max',
            type=int,
            help=('Max number of clients to run (will start at 1, '
                'go through powers of two until max). Default 1.'),
            default=1)
    parser.add_argument('--nservers',
            type=int,
            help='Number of server processes to run. Default 1.',
            default=1)
    parser.add_argument('--start_port',
            type=int,
            help='Base port number. Default 8000.',
            default=8000)
    parser.add_argument('--wdir', 
            help='Working directory. Default /proj/sequencer/tsch/mininetpipe.',
            default='/proj/sequencer/tsch/mininetpipe')
    parser.add_argument('--results_dir',
            help='Directory to put the results file',
            default=None)
    parser.add_argument('--results_file',
            help='File to put the results',
            default=None)

    args = parser.parse_args()

    print "About to start experiment..."

    experiment_set = ExperimentSet(args)
    experiment_set.run_experiments()

