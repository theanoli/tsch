# CD into the directory where DPDK setup lives, or else this will not work
# because of this line:
dpdk_dir=`echo pwd`

tasks=("x86_64-native-linuxapp-gcc" "Insert IGB UIO module" "Setup hugepage mappings for NUMA systems" "Remove IGB UIO module" "Exit Script")

fname=some_random_file.txt
echo 34 | bash setup.sh > $fname 
options=""

for i in "${tasks[@]}"; do
	optnum=$(awk -v pattern="$i" '$0 ~ pattern' $fname | # find line
		awk 'BEGIN { FS="[ ]" }; { print $1 }' |     # get first elem
		grep -o '[0-9]\+') 			     # strip brackets 
	options="$options$optnum\n\n"
done

echo $options
# rm $fname
