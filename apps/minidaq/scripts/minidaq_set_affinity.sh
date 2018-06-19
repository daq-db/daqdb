#!/bin/bash

# Usage
# ./minidaq_set_affinity[tid_file][base_cpu_core][n_cpu_cores]

tFile=$1
baseCore=$2
nCores=$3
i=0

while read tid; do
    core=$(( $baseCore + $i ))
    echo "TID $tid -> core $core"
    taskset -p -c $core $tid &> /dev/null
    i=$(( $i + 1))
    i=$(( $i % $nCores ))
done < $tFile
