# Performs the necessary tasks for setting up DPDK. The tasks are those that
# are done from the setup.sh file in DPDK's tools directory (see the tasks
# array below). 
#
# Must include as arguments
#	1. which Emulab machine type you are using (for hugepage mappings)
# 	2. the interface you want to activate DPDK on.
#
# Usage: 
#	sudo bash dpdk_setup.sh machine_type iface_name  
#
# CD into the directory where DPDK setup lives, or else this will not work
# because of this line:
dpdk_dir=`echo pwd`

node_type=$1
iface=$2

tasks=( "x86_64-native-linuxapp-gcc" "Insert IGB UIO module" "Setup hugepage mappings for NUMA systems" "Bind Ethernet device to IGB UIO module" )

# Files in which to dump options menu of the DPDK setup script (fname)
# and the PCI information (pci_fname)
fname=some_random_file.txt
pci_fname=another_random_file.txt

# Dump the menu contents; "q" will quit, since we don't know exit code yet
printf q | bash setup.sh > $fname

optios=""

# Extract the option number from the setup menu text
for i in "${tasks[@]}"; do
	# Find the correct line and extract the option number
	optnum=$(awk -v pat="$i" '$0 ~ pat { 
				gsub(/\[/,""); 
				gsub(/\]/,""); 
				print $1 
			}' $fname )	
	
	# "Bind Ethernet device..." is a special case; we need to examine
	# a sub-menu, too
	if [ "$i" == "Bind Ethernet device to IGB UIO module" ]; then
		# Expand the submenu and save to file; printf preserves \n
		printf "$optnum\n\nq\n\n" | bash setup.sh > $pci_fname

		optnum="$optnum\n$(awk -v pat="$iface" '$0 ~ pat { 
					gsub(/\[/,""); 
					gsub(/\]/,""); 
					print $1 
				}' $pci_fname )"	
	fi
	
	# Number of times you need to enter mappings depends on node type
	if [ "$i" == "Setup hugepage mappings for NUMA systems" ]; then
		if [ "$node_type" == "d430" ]; then
			numa_string="1024\n1024"
		else
			numa_string="1024\n1024\n1024\n1024"
		fi
		optnum="$optnum\n$numa_string"
	fi

	options="$options$optnum\n\n"
done

echo $options
rm $fname
rm $pci_fname
