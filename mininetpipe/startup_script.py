import os
import sys
import subprocess
import shlex
import argparse

def set_up_clients(client_list):
    # Takes as arg a list of strings
    procs = []
    
    subprocess.Popen(shlex.split("sudo bash machine_startup_script.sh"))

    for client in client_list:
        print("Working on %s..." % client)
        procs.append(subprocess.Popen(
            shlex.split("ssh %s 'cd %s; sudo bash -s' < machine_startup_script.sh"
                % (client, "/proj/sequencer/tsch/mininetpipe/tsch"))))
        for proc in procs:
            proc.wait()

    
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Set up this and client machines "
            "for throughput experiments: turn off hyperthreading, kill collectl, "
            "turn off irqbalance.")
    parser.add_argument("--clients",
            help="List of client machine IP addresses or names.")

    args = parser.parse_args()

    set_up_clients([client for client in args.clients.split(",")])
