iface=$1
ncores=$2
do_flowdir=$3
do_rfs=$4

# Set up flow director
# Specify actions for matches
# To check: sudo ethtool -u <interface>
# This specifies which RX queue should handle a packet destined/sourced
# for a particular port. 
if [[ $do_flowdir -eq 1 ]]; then
    # Enable ntuple hashing
    sudo ethtool -K $iface ntuple on

    for x in `seq 0 256`; do  # max number of client threads
        port=$((8000 + $x))
        queue=$(($x % $ncores))
        echo "Sending port $port packets to queue $queue"
        sudo ethtool -N $iface flow-type tcp4 \
            dst-port $port \
            action $queue
    done
fi

# Set up rfs: Steer packets to core running application once processed
if [[ $do_rfs -eq 1 ]]; then 
    # Set max number of entries
    echo 32768 | sudo tee /proc/sys/net/core/rps_sock_flow_entries

    # Set this value to be the max number of entries/total # queues
    cd /sys/class/net/$iface/queues
    for queue in rx-*; do
        echo 1024 | sudo tee $queue/rps_flow_cnt
    done
fi
