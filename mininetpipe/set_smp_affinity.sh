iface=$1
smp_start=`cat /proc/interrupts | grep $iface-TxRx-0 | awk '{ print $1 }' | sed 's/://g'`

echo $smp_start

for i in {0..31}; do
    sudo echo $i > /proc/irq/$(($smp_start + $i))/smp_affinity_list
done
