#! /usr/bin/python

import os
import re
import sys
import argparse
import subprocess
import time
import shlex

class ExperimentSet(object):
    def __init__(self, args):
        self.printlabel = "[" + os.path.basename(__file__) + "]"

        self.serv_basecmd = args.serv_basecmd
        self.cli_basecmd = args.cli_basecmd
        self.nodes = [".".join([x, "drat.sequencer.emulab.net"])
                if "client" in x else x
                for x in args.nodes.split(",")]
        self.ntrials = args.ntrials
        self.ncli_min = args.ncli_min
        self.ncli_max = args.ncli_max
        self.nservers = args.nservers
        self.start_port = args.start_port
        self.wdir = args.wdir
        self.start_port = args.start_port

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
        self.nprocs_per_client = nprocs
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
            serv_cmd = (self.serv_basecmd +
                    " -c %d" % self.total_clientprocs + 
                    " -P %d" % (8000 + i))
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
            nodecmds = "cd %s; pwd; " % self.wdir
            for n in range(self.nprocs_per_client):
                if n % 100 == 0:
                    self.printer("Launching client process %d on %s" % 
                            (n, node))
                cmd = (self.cli_basecmd + 
                        " -P %d" % (8000 + (n % self.nservers)))
                nodecmds += (cmd + " & ")
            nodecmds = nodecmds[:-2]
            subprocess.Popen(shlex.split("ssh %s '%s'" % (node, nodecmds)))
        
    def run_experiment(self):
        self.total_clientprocs = self.nprocs_per_client * len(self.nodes)

        self.kill_zombie_processes()

        servers = self.launch_servers()
        self.launch_clients()

        for server in servers:
            server.wait()

    
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run one or more server processes '
            'and multiple client processes.')
    parser.add_argument('serv_basecmd',
            help=('Command to run for each server process. Will be augmented '
            'with port number and total number of clients.'))
    parser.add_argument('cli_basecmd',
            help='Command to run for each client process.')
    parser.add_argument('nodes', 
            help='List of client machine IP addresses or names.')
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


    args = parser.parse_args()

    print "About to start experiment..."

    experiment_set = ExperimentSet(args)
    experiment_set.run_experiments()

