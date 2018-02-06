for node in $@; do
    ssh $node 'bash -s -- eth4 eth5' < machine_startup_script.sh
done
