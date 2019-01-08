# Spread interrupt handling for all queues across all CPUs
iface=$1
ncores=$2

cpu=0

irq=`cat /proc/interrupts | grep $iface-TxRx-0 | awk '{ print $1 }' | sed 's/://g'`

cd /sys/class/net/$iface/device/msi_irqs/
for cpu in `seq 0 $(($ncores-1))`; do
    echo "$cpu for irq $irq"
    sudo echo $cpu > /proc/irq/$irq/smp_affinity_list
    irq=$(($irq + 1))
done
