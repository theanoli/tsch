import run_experiments
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run a single server process'
            'and multiple client processes.')
    parser.add_argument('nclients_range',
            type=int,
            help=('End range of nclients to run (will start at 1, '
                'go through powers of two)'))
    parser.add_argument('ntrials',
            type=int,
            help='Number of times to repeat experiment')
    parser.add_argument('serv_cmd',
            help='Command to run for each server process.')
    parser.add_argument('cli_cmd',
            help='Command to run for each client process.')
    parser.add_argument('nodes', 
            help='List of client machine IP addresses or names.')

    args = parser.parse_args()

    nclients_range = args.nclients_range
    ntrials = args.ntrials
    cli_cmd = args.cli_cmd
    serv_cmd = args.serv_cmd
    nodes = [".".join([x, "drat.sequencer.emulab.net"]) 
            for x in args.nodes.split(",")]
    
    n = 1
    while n <= nclients_range:
        for trial in range(ntrials):
            print ""
            print "***Running trial %d for nclients=%d!***" % (trial + 1, n)
            run_experiments.run_experiment(cli_cmd, serv_cmd, nodes, n)
            print "***Completed trial %d!***" % (trial + 1)
            print ""
        n *= 2
