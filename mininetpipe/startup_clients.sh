for node in $@; do
    ssh $node 'bash -s -- `ifconfig | awk '\''{ print $1 }'\'' | grep eth | grep -v eth0`' < machine_startup_script.sh
done
