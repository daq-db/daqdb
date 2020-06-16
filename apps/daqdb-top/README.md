# DAQDB-TOP: Toolkit for mOnitoring Performance


## mention this is a lightweight version of minidaq



## automate testing


number of events assigned to each thread is equal to the number of events divided by the number of execution threads

Setting fixed componentId and changing the eventI

## will add performance monitoring 

## quick test

yes | rm /mnt/pmem/*.pm



sudo ./producer --time-test 90000000 --time-iter 100 --time-ramp 0 --pmem-size 6005617664 --pmem-path /mnt/pmem/fogkv_minidaq.pm --n-poolers 0 --base-core-id 5 --n-cores 29 --start-delay 0 --acceptance 0.0 --fragment-size 1000 -s --stopOnError --max-ready-keys 0 --n-dht-threads 1 --n-ro 1 --readout-cores 22 --config-file server.cfg --number-events 1000000


