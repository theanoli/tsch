#! /bin/bash
export RTE_SDK=$PWD

HUGEPGSZ=`cat /proc/meminfo  | grep Hugepagesize | cut -d : -f 2 | tr -d ' '`

#
# Creates hugepage filesystem.
#
create_mnt_huge()
{
	echo "Creating /mnt/huge and mounting as hugetlbfs"
	sudo mkdir -p /mnt/huge

	grep -s '/mnt/huge' /proc/mounts > /dev/null
	if [ $? -ne 0 ] ; then
		sudo mount -t hugetlbfs nodev /mnt/huge
	fi
}

#
# Removes hugepage filesystem.
#
remove_mnt_huge()
{
	echo "Unmounting /mnt/huge and removing directory"
	grep -s '/mnt/huge' /proc/mounts > /dev/null
	if [ $? -eq 0 ] ; then
		sudo umount /mnt/huge
	fi

	if [ -d /mnt/huge ] ; then
		sudo rm -R /mnt/huge
	fi
}

#
# Removes all reserved hugepages.
#
clear_huge_pages()
{
	echo > .echo_tmp
	for d in /sys/devices/system/node/node? ; do
		echo "echo 0 > $d/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" >> .echo_tmp
	done
	echo "Removing currently reserved hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	remove_mnt_huge
}

#
# Creates hugepages on specific NUMA nodes.
#
set_numa_pages()
{
	clear_huge_pages

	Pages=2048

	echo > .echo_tmp
	for d in /sys/devices/system/node/node? ; do
		node=$(basename $d)
		echo "echo $Pages > $d/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" >> .echo_tmp
	done
	echo "Reserving hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	create_mnt_huge
}

#
# Print hugepage information.
#
grep_meminfo()
{
	grep -i huge /proc/meminfo
}

echo "------------------------------------------------------------------------------"
echo " Setting up hugepages..." 
echo "------------------------------------------------------------------------------"
set_numa_pages

# Set up hugepages
sudo bash -c "echo kernel.shmmax = 9223372036854775807 >> /etc/sysctl.conf"
sudo bash -c "echo kernel.shmall = 1152921504606846720 >> /etc/sysctl.conf"
sudo sysctl -p /etc/sysctl.conf
