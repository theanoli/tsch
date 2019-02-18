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
        iface = iface.decode().strip()
        if not iface:
            return None, None
    except:
        return None, None

    cmd = ssh + """ \"ifconfig %s | awk -F ' *|:' '/inet addr/{ print $4 }'\"""" % iface
    try:
        ip = subprocess.check_output(shlex.split(cmd))
        ip = ip.decode().strip()
        if not ip:
            return None, None
    except:
        return None, None
    
    return ip, iface
    

def get_mac(ssh, iface):
    cmd = ssh + """ \"ifconfig -a %s | awk 'NR==1{ print $5 }'\"""" % iface
    if iface is None:
        return None
    try: 
        mac = subprocess.check_output(shlex.split(cmd))
        return mac.decode().strip()
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
        machinename = fields[0]  # e.g., "client-0"
        machineid = fields[1]  # e.g., "pc722"

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
            print("Couldn't get iface and/or mac--will pause before setting" +
                " up DPDK so you can input them manually")

        machines[machinename]['iface'] = iface
        machines[machinename]['mac'] = mac
        machines[machinename]['ip'] = ip
        
        dump.write("%s,%s,%s,%s,%s\n" % (machinename, machineid, iface, mac, ip))
        print(machinename, machineid, iface, mac, ip)

    f.close()
    dump.close()

    if manual_input_flag:
        input ("Pausing for manual input; save machine_list.txt with" + 
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


def setup_machines(whoami, args):
    home = os.getcwd()
    codes = []
    for i, machine in enumerate(machines.keys()):
        machineid = machines[machine]['machineid']
        if "server" in machine:
            am_server = 1
        else:
            am_server = 0

        if args.do_mtcp:
            print("\nSetting up mTCP on all machines...")
            ip_last_octet = i+1
            # Takes as args: iface, integer (for IP address)
            setup_cmd = ("cd %s; sudo bash mtcp_setup.sh %s %d" % 
                    (home, machines[machine]['iface'], ip_last_octet))
            machines[machine]['ip'] = '10.0.0.%d' % ip_last_octet

        elif args.do_dpdk:
            print("\nSetting up DPDK on all machines...")
            # Takes as arg: iface
            setup_cmd = ("cd %s; source 'dpdk_setup.sh %s'" % 
                    (home, machines[machine]['iface']))

        elif args.do_udp:
            print("\nSetting up all machines for UDP...")
            setup_cmd = ("cd %s; sudo bash udp_setup.sh %s %s" %
                    (home, machines[machine]['iface'],
                        am_server))

        elif args.do_tcp:
            print("\nSetting up all machines for TCP...")
            setup_cmd = ("cd %s; sudo bash tcp_setup.sh %s %s" %
                    (home, machines[machine]['iface'],
                        am_server))

        else: 
            print("\nNo additional setup! Exiting...")
            return

        ssh = "ssh %s@%s.emulab.net" % (whoami, machineid)
        setup_cmd = "%s '%s'" % (ssh, setup_cmd)
        p = subprocess.Popen(shlex.split(setup_cmd))
        
        # This needs to happen serially so compilation doesn't collide
        p.wait()
        codes.append((machine, p.returncode))

    # Need to update the IP addresses of all machines
    if args.do_mtcp:
        with open('machine_info.txt', 'wb') as dump:
            for machine, values in machines.items():
                info = "%s,%s,%s,%s,%s\n" % (
                    machine, 
                    values['machineid'], 
                    values['iface'], 
                    values['mac'], 
                    values['ip'])
                dump.write(info.encode('utf-8'))

    for machine, rc in codes: 
        print("%s completed with exit status %d" % 
            (machine, rc))


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
    parser.add_argument('--do_udp',
            help=('If set, set up RSS/RFS on all machines.'),
            action='store_true')
    parser.add_argument('--do_tcp',
            help=('If set, disable hyperthreading on all machines.'),
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

    setup_machines(args.whoami, args)
