#!/bin/bash

# Example script to run minidaq benchmark. Assumes root privileges.

# Input argumentkjs
test_time=800
ramp_time=200
iter_time=10
prefix=nth
ncores=17
fogkv_dir=~/FogKV
fogkv_psize=10737418240
fogkv_pfile=/mnt/pmem/fogkv_minidaq.pm
fogkv_poolers=0
fogkv_base_core_id=1
minidaq_iter_arg="--n-ro"
minidaq_node_args="--fragment-size 1024"
iters=(1 2)
dry_run=0

# Internal variables
work_dir=`pwd`
results_file=$work_dir/summary.csv
env_file=$work_dir/env.log
minidaq_fogkv_args="--time-test $test_time --time-iter $iter_time --time-ramp $ramp_time\
 --pmem-size $fogkv_psize --n-poolers $fogkv_poolers --base-core-id $fogkv_base_core_id\
 --n-cores $ncores"
minidaq_args="$minidaq_args $minidaq_fogkv_args $minidaq_node_args --out-summary $results_file"

echo -e "#######################################"
echo -e "########## MINIDAQ BENCHMARK ##########"
echo -e "#######################################"

# Dump configuration
date=$(date '+%Y%m%d_%H%M%S')
cd $fogkv_dir
echo "System date: $date" > $env_file
echo "Git commit: `git rev-parse HEAD`" >> $env_file
echo "Git tag: `git describe --tags 2> /dev/null`" >> $env_file
cd $work_dir
echo "##### Filesystems #####" >> $env_file
df -h >> $env_file
echo "##### Memory #####" >> $env_file
cat /proc/meminfo >> $env_file
echo "##### CPU #####" >> $env_file
cat /proc/cpuinfo >> $env_file
echo "##### boot #####" >> $env_file
cat /etc/default/grub >> $env_file
echo "##### kernel #####" >> $env_file
uname -a >> $env_file
echo "##### minidaq #####" >> $env_file
echo "$minidaq_args $minidaq_iter_arg X" >> $env_file

# Configure environment
echo -e "[$date] setting environment..."
rm -f *.csv
. $fogkv_dir/scripts/setup_env_lcg.sh
cd $fogkv_dir/bin
echo -e "[$date] done\n"

echo -e "[$date] starting...\n"

for i in `seq 0 $((${#iters[*]} - 1))`; do
	date=$(date '+%Y%m%d_%H%M%S')
	echo -e "[$date] $prefix ${iters[$i]}"
	rm -rf $pfile 2> /dev/null
	test_name=${iters[$i]}
	iteration_dir=$work_dir/$prefix${iters[$i]}
	args="$minidaq_args --out-prefix $iteration_dir --test-name ${iters[$i]}"
	args="$args $minidaq_iter_arg ${iters[$i]}"
	if [ $dry_run -ne 0 ]
	then
		echo "[$date] ./minidaq $args"
	else
		./minidaq $args
	fi

	echo -e "\n"
done

cd $work_dir
echo -e "[$date] done"
