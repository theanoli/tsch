#! /usr/bin/python

import os
import re
import sys

import atexit
import argparse
import math
import shlex
import subprocess
import time

wait_multiplier = 0

class Experiment(object):
    def __init__(self, args):
        self.printlabel = "[" + os.path.basename(__file__) + "]"

        self.basecmd = args.basecmd
        self.whoami = args.whoami
        self.do_tput = args.do_tput
        self.server_addr = args.server_addr
        self.nclient_threads = args.nclient_threads
        self.nclient_machines = args.nclient_machines
        self.nservers = args.nservers
        self.start_port = args.start_port
        self.wdir = args.wdir

        self.outfile = args.outfile
        self.outdir = args.outdir

        self.online_wait = args.online_wait
        self.wait_multiplier = args.wait_multiplier
        self.no_collect_stats = args.no_collect_stats
        self.expduration = args.expduration
        self.no_pin_procs = args.no_pin_procs

        self.machine_dict, self.clients, self.servers = self.read_machine_info()
        self.clients = self.clients[:self.nclient_machines]

        timestamp = int(time.time())
        if self.outfile == None:
            self.outfile = "results"

        if self.outdir == None:
            self.outdir = os.path.join(self.wdir, "results", "results-%d" % 
                    timestamp)

        if not os.path.isdir(self.outdir):
            os.mkdir(self.outdir)

        if self.do_tput:
            self.basecmd += " -t"
        else:
            self.expduration = 100000 # some arbitrarily large number

        self.printer("Number of client threads: %d" % 
                (self.nclient_threads * len(self.clients)))

        self.total_clients = self.nclient_threads * len(self.clients)

        # Dump all the class variables (experiment settings) to file
        with open("%s/config.txt" % self.outdir, 'w') as f:
            for var, value in self.__dict__.iteritems():
                f.write("%s: %s\n" % (var, value))
        
        self.kill_zombie_processes()
        atexit.register(self.kill_zombie_processes)
        
    def printer(self, msg): 
        print "%s %s" % (self.printlabel, msg)
    
    def kill_zombie_processes(self):
        # Clean up any potential zombies on the client side(s)
        self.printer("Killing zombie processes on client(s).")
        killers = []
        for machine in self.machine_dict.keys(): 
            killers.append(subprocess.Popen(
                shlex.split("ssh -p 22 %s@%s.emulab.net 'sudo pkill \"NP[a-z]*\"'" % 
                        (self.whoami, self.machine_dict[machine]['machineid']))))
        for killer in killers:
            killer.wait()

    def read_machine_info(self):
        clients = []
        servers = []
        machines = {}

        f = open("../machine_info.txt", "r")

        for line in f.readlines():
            fields = line.split(',')
            machinename = fields[0]
            machineid = fields[1]
            iface = fields[2]
            mac = fields[3]

            if "server" in machinename:
                servers.append(machinename)
            else:
                clients.append(machinename)
            
            machines[machinename] = {
                    'machineid': machineid,
                    'iface': iface,
                    'mac': mac,
            }

        f.close()
        return machines, sorted(clients), sorted(servers)

    def launch_servers(self):
        # Launch the server-side program and then wait a bit 
        self.printer("Launching servers...")
        
        os.chdir(self.wdir)

        serv_cmd = (self.basecmd +
                # -c should be the per-server thread #clients
                " -c %d" % (self.total_clients/self.nservers) +  
                " -P %d" % self.start_port +
                " -w %d" % self.online_wait +
                " -d %s" % self.outdir + 
                " -o %s" % self.outfile +
                " -u %d" % self.expduration + 
                " -T %d" % self.nservers)
        if not self.no_pin_procs: 
            serv_cmd += " -p"
        if not self.no_collect_stats:
            serv_cmd += " -l"
        cmd = shlex.split(serv_cmd)
        self.printer("Launching server process: %s" % serv_cmd)
        server = subprocess.Popen(cmd)

        return server

    def launch_clients(self):
        # Launch the client-side programs
        self.printer("Launching clients...")
        i = 0
        subprocesses = []

        for client in self.clients:
            cmd = (self.basecmd + 
                    " -c %d" % self.nservers + 
                    " -H %s" % self.server_addr +
                    " -d %s" % self.outdir + 
                    " -o %s" % self.outfile +
                    " -T %d" % (self.nclient_threads) +
                    " -P %d" % self.start_port)

            ssh = "ssh -p 22 %s@%s.emulab.net 'cd %s; %s'" % (self.whoami, 
                    self.machine_dict[client]['machineid'], self.wdir, cmd)
            self.printer("Sending command to client %s: %s" % (client, ssh))
            p = subprocess.Popen(shlex.split(ssh))
            subprocesses.append(p)
        return subprocesses
 
    
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run one or more server processes '
            'and multiple client processes.')
    parser.add_argument('basecmd',
            help=('Command to run for each process. Will be augmented '
            'with port number and total number of clients for server, '
            'server IP and port number for clients.'))
    parser.add_argument('whoami',
            help=('Emulab username for SSH'))
    parser.add_argument('--do_tput',
            help=('Run throughput experiment(s)'),
            action='store_true')
    parser.add_argument('--server_addr',
            help=('Address of the server. Default server-0'),
            default="server-0") 
    parser.add_argument('--nclient_threads',
            type=int,
            help=('Number of client threads to run per client machine. '
                'Default 1.'),
            default=1)
    parser.add_argument('--nclient_machines',
            type=int,
            help=("Number of client machines to launch. Default 1."),
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
    parser.add_argument('--outdir',
            help=('Directory to write results to.'),
            default=None)
    parser.add_argument('--online_wait',
            help=('Time to wait for clients to come online. Default '
                'is %f times the total number of clients.' % wait_multiplier),
            type=int,
            default=0)
    parser.add_argument('--wait_multiplier',
            help=('Multiplier for how long to wait for clients to come '
                'online. Default is %f.' % wait_multiplier),
            type=float,
            default=wait_multiplier)
    parser.add_argument('--no_collect_stats',
            help=('Collect stats about CPU usage.'),
            action='store_true')
    parser.add_argument('--expduration',
            type=int,
            help=('Set throughput experiment duration. Default 20s.'),
            default=20)
    parser.add_argument('--no_pin_procs',
            help=('Pin each process to a core.'),
            action='store_true')
    parser.add_argument('--outfile',
            help=('Base name for experiments'),
            default=None)
    

    args = parser.parse_args()

    experiment = Experiment(args)
    experiment.printer("Running experiment!")
    experiment.kill_zombie_processes()
    server = experiment.launch_servers()
    time.sleep(2)
    clients = experiment.launch_clients()
    for p in clients:
            p.wait()
    experiment.printer("Completed experiment!")

