# Takes as input the copy-pasted list of machines in the "List View" tab of
# Emulab experiment page (must be titled "machine_list.txt" in top-level dir)
# and sets up Emulab on proxies, sequencer, and DT(s). 
# Creates a file with each machine's ID, Node#, interface (only relevant for
# clients since it will be brought down for DPDK machines), and MAC address.
import os
import subprocess
import shlex
import argparse


def get_iface(ssh):
    cmd = ssh + """ \"ifconfig | grep 10\\.1\\.1 -B1 | awk 'NR==1{ print $1 }'\""""
    try:
        iface = subprocess.check_output(shlex.split(cmd))
        if not iface.strip():
            return None, None
    except:
        return None, None

    cmd = ssh + """ \"ifconfig %s | awk -F ' *|:' '/inet addr/{ print $4 }'\"""" % iface.strip()
    try:
        ip = subprocess.check_output(shlex.split(cmd))
        if not ip.strip():
            return None, None
    except:
        return None, None
    
    return ip.strip(), iface.strip()
    

def get_mac(ssh, iface):
    cmd = ssh + """ \"ifconfig -a %s | awk 'NR==1{ print $5 }'\"""" % iface
    if iface is None:
        return None
    try: 
        mac = subprocess.check_output(shlex.split(cmd))
        return mac.strip()
    except: 
        return None

def get_machine_info():
    clients = []
    servers = []
    machines = {}

    manual_input_flag = 0

    f = open("machine_list.txt", "r")
    dump = open("machine_info.txt", "w")

    for line in f.readlines():
        if line == '\n':
            continue
        fields = line.split()
        machinename = fields[0]
        machineid = fields[1]

        if "server" in machinename:
            servers.append(machinename)
        else: 
            clients.append(machinename)
        
        machines[machinename] = {
                'machineid': machineid
        }

        ssh = " ".join(fields[5:])
        print(ssh)

        ip, iface = get_iface(ssh)
        mac = get_mac(ssh, iface)

        if iface is None or mac is None:
            manual_input_flag = 1
            print ("Couldn't get iface and/or mac--will pause before setting" +
                " up DPDK so you can input them manually")

        machines[machinename]['iface'] = iface
        machines[machinename]['mac'] = mac
        machines[machinename]['ip'] = ip
        
        dump.write("%s,%s,%s,%s,%s\n" % (machinename, machineid, iface, mac, ip))
        print machinename, machineid, iface, mac, ip

    f.close()
    dump.close()

    if manual_input_flag:
        raw_input ("Pausing for manual input; save machine_list.txt with" + 
                " missing values filled in, then hit any key to keep going... ")
        machines, clients, servers = read_machine_info()

    return machines, clients, servers


def read_machine_info():
    clients = []
    servers = []
    machines = {}

    f = open("machine_info.txt", "r")

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
    return machines, clients, servers


def setup_dpdk(whoami, machine_dict, do_mtcp):
    home = os.getcwd()
    codes = []
    for i, machine in enumerate(machine_dict.keys()):
        machineid = machines[machine]['machineid']
        if do_mtcp:
            # Takes as args: iface, integer (for IP address)
            setup_cmd = ("cd %s; sudo bash mtcp_setup.sh %s %d" % 
                    (home, machines[machine]['iface'], i))
        else:
            # Takes as arg: iface
            setup_cmd = ("cd %s; source 'dpdk_setup.sh %s'" % 
                    (home, machines[machine]['iface']))
        ssh = "ssh %s@%s.emulab.net" % (whoami, machineid)
        setup_cmd = "%s '%s'" % (ssh, setup_cmd)
        p = subprocess.Popen(shlex.split(setup_cmd))
        p.wait()
        codes.append((machine, p.returncode))

    for machine, rc in codes: 
        print ("%s completed with exit status %d" % 
            (machine, rc))
        

def setup_rss_rfs(whoami, machine_dict):
    print "\nSetting up RSS/RFS on all machines..."
    subprocesses = []
    for machine in machine_dict.keys():
        if "server" in machine: 
            am_server = 1
        else:
            am_server = 0

        print ("\nSetting up RSS/RFS on machine %s (%s)..." % (
                machine,
                machines[machine]['machineid']))

        script_path = os.path.join(os.getcwd())
        ssh = ("ssh %s@%s.emulab.net" % (
            args.whoami, machines[machine]['machineid']))
        setup_cmd = ("cd %s; sudo bash machine_startup_script.sh %s %s" %
                (script_path, 
                    machines[machine]['iface'],
                    am_server))
        print(setup_cmd)
        setup_cmd = "%s '%s'" % (ssh, setup_cmd)
        p = subprocess.Popen(shlex.split(setup_cmd))
        p.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Set up all machines.')
    parser.add_argument('whoami',
            help=('Emulab username'))
    parser.add_argument('--do_dpdk',
            help=('If set, set up DPDK on all machines.'),
            action='store_true')
    parser.add_argument('--do_mtcp',
            help=('If set, set up mTCP on all machines.'),
            action='store_true')
    parser.add_argument('--do_rss',
            help=('If set, set up RSS/RFS on all machines.'),
            action='store_true')
    parser.add_argument('--read_info',
            help=('If set, read machine info from machine_info.txt rather than' +
                ' parsing from file.'),
            action='store_true')

    args = parser.parse_args()

    if args.read_info:
        machines, clients, servers = read_machine_info()
    else:
        machines, clients, servers = get_machine_info()

    if args.do_dpdk or args.do_mtcp:
        setup_dpdk(args.whoami, machines, args.do_mtcp)
    if args.do_rss:
        setup_rss_rfs(args.whoami, machines)
