from run_experiments import run_experiment
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run one or more server processes '
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
    parser.add_argument('nclients_max',
            type=int,
            help=('Max number of clients to run (will start at 1, '
                'go through powers of two until max)'))
    parser.add_argument('--nservers',
            type=int,
            help='Number of server processes to run.',
            default=1)

    args = parser.parse_args()

    nclients_max = args.nclients_max
    ntrials = args.ntrials
    cli_cmd = args.cli_cmd
    serv_cmd = args.serv_cmd
    nodes = [".".join([x, "drat.sequencer.emulab.net"]) 
            for x in args.nodes.split(",")]
    nservers = args.nservers
    
    n = nservers
    while n <= nclients_max:
        for trial in range(ntrials):
            print ""
            print "***Running trial %d for nclients=%d!***" % (trial + 1, n)
            run_experiment(cli_cmd, serv_cmd, nodes, n, nservers)
            print "***Completed trial %d!***" % (trial + 1)
            print ""
        n *= 2
