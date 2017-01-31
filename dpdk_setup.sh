# Performs the necessary tasks for setting up DPDK. The tasks are those that
# are done from the setup.sh file in DPDK's tools directory (see the tasks
# array below). 
#
# Must include as argument the interface you want to activate DPDK on. Usage: 
#	sudo bash dpdk_setup.sh iface_name  
#
# Note the setup.sh program is extremely sensitive to misplaced newlines!
# If it seems to be working incorrectly, echo $options to see what you're
# piping and make sure it is exactly as you'd manually enter it to the program.
#
# CD into the directory where DPDK setup lives, or else this will not work
# because of this line:
dpdk_dir=`echo pwd`

# Find the DPDK-compatible interface and take it down
iface=`ifconfig | awk '!/10\.1\.1\..*/ {iface=$1}
			/10\.1\.1\..*/ {print iface}'`

if [ $iface == "" ]; then
	echo "Error getting interface name! Exiting..."
	exit
fi

# Get the number of NUMA nodes for this machine type
num_numa_nodes=$( lscpu | awk -F': +' '/NUMA node\(s\)/ { print $2 }' )

tasks=( "x86_64-native-linuxapp-gcc" 
	"Insert IGB UIO module" 
	"Setup hugepage mappings for NUMA systems" 
	"Bind Ethernet device to IGB UIO module" )

# Files in which to dump options menu of the DPDK setup script (fname)
# and the PCI information (pci_fname)
fname=some_random_file.txt
pci_fname=another_random_file.txt

# Dump the menu contents; "q" will quit, since we don't know exit code yet
printf q | bash setup.sh > $fname

# Extract the option number from the setup menu text
for i in "${tasks[@]}"; do
	# Find the correct line and extract the option number
	optnum=$(awk -v pat="$i" '$0 ~ pat { 
				gsub(/\[/,""); 
				gsub(/\]/,""); 
				print $1 
			}' $fname )	
	
	# "Bind Ethernet device..." is a special case; we need to examine
	# a sub-menu, too. This is ugly but it works and faster than 
	# alternative (lshw is really slow!)
	if [ "$i" == "Bind Ethernet device to IGB UIO module" ]; then
		# Expand the submenu and save to file; printf preserves \n
		printf "$optnum\nq\nq\nq\n" | bash setup.sh > $pci_fname

		optnum="$optnum\n$(awk -v pat="$iface" '$0 ~ pat { print $1 }' $pci_fname | 
				awk -F':' '{ print $2":"$3 }')"
	fi
	
	# Add an input for every NUMA node
	if [ "$i" == "Setup hugepage mappings for NUMA systems" ]; then
		for i in `seq 1 $num_numa_nodes`; do
			numa_string="$numa_string\n1024"
		done

		optnum="$optnum$numa_string"
	fi

	options="$options$optnum\n\n"
done

printf "\n\n++++++++++++++++++++++++++++++++++++++++++++"
printf "Interface $iface will be configured for DPDK.\n"
printf "Your setup options will be:\n"
printf "$options"
read -n 1 -s -p "Press any key to continue, Ctrl+C to exit..."

echo "Bringing down interface $iface..."
ifconfig $iface down

printf "$options q\n" | bash setup.sh
rm $fname
rm $pci_fname
