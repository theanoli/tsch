#! /usr/bin/python

import os
import re
import sys

import argparse
import math
import shlex
import subprocess
import time

wait_multiplier = 0

class ExperimentSet(object):
    def __init__(self, args):
        self.printlabel = "[" + os.path.basename(__file__) + "]"

        self.basecmd = args.basecmd
        self.server_addr = args.server_addr
        self.nodes = args.nodes.split(",")
        self.ntrials = args.ntrials
        self.nclients = args.nclients
        self.nservers = args.nservers
        self.start_port = args.start_port
        self.wdir = args.wdir
        self.results_dir = args.results_dir
        self.online_wait = args.online_wait
        self.wait_multiplier = args.wait_multiplier
        self.collect_stats = args.collect_stats
        self.expduration = args.expduration
        self.pin_procs = args.pin_procs

        try:
            self.results_filebase = (args.results_filebase + 
                                    "_u%d" % self.expduration)
        except:
            self.results_filebase = None 

        # If we have more server threads than clients, override the number of
        # clients to ensure each server thread gets at least one client proc
        if self.nclients < self.nservers:
            self.nclients = self.nservers
        
    def printer(self, msg): 
        print "%s %s" % (self.printlabel, msg)
    
    def run_experiments(self):
        # Launch the entire set of experiments for this experiment set
        # TODO un-hardcode this at some point
        n = 1 

        while n < 8:
            for trial in range(self.ntrials):
                self.printer("Running trial %d for %d clients per node" % 
                        (trial + 1, n))
                experiment = Experiment(self, n, trial)
                self.printer("Completed trial %d!" % (trial + 1))
                time.sleep(2)
                n *= 2
            self.printer("Moving on to %d clients per node." % n)


class Experiment(object):
    def __init__(self, experiment_set, nthreads, trial_number):
        self.experiment_set = experiment_set
        self.trial_number = trial_number
        self.clients = nthreads
        self.printer("Number of client threads: %d" % (nthreads))

        if self.results_filebase:
            # Override the trial number to keep going from last trial
            if self.results_dir is not None:
                previous_trials = [x for x in 
                    os.listdir(os.path.join("results", self.results_dir))
                    if self.results_filebase in x]
                print previous_trials

                if len(previous_trials) > 0:
                    last_trials = [int(re.search(
                        r"^[0-9a-z_]+_(\d+)\.dat$", x).group(1))
                        for x in previous_trials if ".tab" not in x]
                    last_trial = max(last_trials)

                    self.trial_number = last_trial + 1

                self.results_file = (self.results_filebase +
                        "_c%d" % self.nthreads * len(self.nodes) + 
                        "_%d.dat" % self.trial_number)
        else: 
            self.results_file = "temp"

        if self.online_wait is None:
            self.online_wait = 0

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

        serv_cmd = (self.basecmd +
                " -c %d" % self.nclients * len(self.nodes) +  # Fix this for TCP
                " -P %d" % self.start_port +
                " -w %d" % self.online_wait +
                " -u %d" % self.expduration + 
                " -T %d" % self.nservers)
        if self.pin_procs: 
            serv_cmd = " -p"
        if self.results_dir:
            serv_cmd += " -d %s" % self.results_dir
        if self.results_file:
            serv_cmd += " -o %s" % self.results_file
        if self.collect_stats:
            serv_cmd += " -l"
        cmd = shlex.split(serv_cmd)
        self.printer("Launching server process: %s" % serv_cmd)
        server = subprocess.Popen(cmd)

        time.sleep(0.5)
        return server

    def launch_clients(self):
        # Launch the client-side programs
        self.printer("Launching clients...")
        i = 0

        for node in self.nodes:
            # Create a giant list of all the commands to be executed on a
            # single node; this avoids opening a ton of SSH sessions
            nodecmd = ""
            cmd = (self.basecmd + 
                    " -H %s" % self.server_addr +
                    " -T %d" % self.nclients + 
                    " -P %d" % self.start_port)

            # TODO this may no longer be necessary since commands are much simpler now
            # Commands can be really long; dump to file
            fname = "cmdfile_%s.sh" % node
            f = open(fname, "w")
            f.write(nodecmd)
            f.close()

        for node in self.nodes:
            self.printer("Contacting client %s to launch client processes..." % node) 
            fname = "cmdfile_%s.sh" % node
            subprocess.Popen(shlex.split("ssh %s 'cd %s; bash -s' < %s" % 
                (node, self.wdir, fname)))
        
    def run_experiment(self):
        self.kill_zombie_processes()

        server = self.launch_servers()
        self.launch_clients()

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
    parser.add_argument('--server_addr',
            help=('Address of the server. Default server-0'),
            default="server-0") 
    parser.add_argument('--ntrials',
            type=int,
            help='Number of times to repeat experiment. Default 1.',
            default=1)
    parser.add_argument('--nclients',
            type=int,
            help='Number of client threads to run. Default 1.',
            default=1)
    parser.add_argument('--nservers',
            type=int,
            help='Number of server threads to run. Default 1.',
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
    parser.add_argument('--results_filebase',
            help='File to put the results',
            default=None)
    parser.add_argument('--online_wait',
            help=('Time to wait for clients to come online. Default '
                'is %f times the total number of clients.' % wait_multiplier),
            type=int,
            default=None)
    parser.add_argument('--wait_multiplier',
            help=('Multiplier for how long to wait for clients to come '
                'online. Default is %f.' % wait_multiplier),
            type=float,
            default=wait_multiplier)
    parser.add_argument('--collect_stats',
            help=('Collect stats about CPU usage.'),
            action='store_true')
    parser.add_argument('--expduration',
            type=int,
            help=('Set throughput experiment duration. Default 20s.'),
            default=20)
    parser.add_argument('--pin_procs',
            help=('Pin each process to a core.'),
            action='store_true')
    

    args = parser.parse_args()

    print "About to start experiment..."

    experiment_set = ExperimentSet(args)
    experiment_set.run_experiments()

