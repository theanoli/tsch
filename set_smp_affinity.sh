# Spread interrupt handling for all queues across all CPUs
iface=$1
ncores=$2

irq=`cat /proc/interrupts | grep $iface-TxRx-0 | awk '{ print $1 }' | sed 's/://g'`

cd /sys/class/net/$iface/device/msi_irqs/
for i in `seq 0 $(($ncores*2 - 1))`; do
    cpu=$(($i % ncores))
    echo "CPU $cpu for irq $irq"
    sudo echo $cpu > /proc/irq/$irq/smp_affinity_list
    irq=$(($irq + 1))
done
