# Set up RSS -- note there are only 63 queues!

# Enable ntuple hashing
sudo ethtool -K eth5 ntuple on

# Specify actions for matches
# To check: sudo ethtool -u <interface>
for x in `seq 0 62`; do
    if [ $x -gt 9 ]; then
        port=80$x
    else
        port=800$x
    fi
    sudo ethtool -N eth5 flow-type tcp4 dst-port $port action $x
done

# Set up RFS
# Set max number of entries
echo 32768 | sudo tee /proc/sys/net/core/rps_sock_flow_entries

# Set this value to be the max number of entries/total # queues
for x in `seq 0 62`; do
    echo 512 | sudo tee /sys/class/net/eth5/queues/rx-$x/rps_flow_cnt
done

