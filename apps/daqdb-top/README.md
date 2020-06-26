
# DAQDB-TOP: Toolkit for mOnitoring Performance

Author: Adam Abed Abud

Last update: June 18, 2020

 ---


DAQDB-TOP is a lightweight tool used to monitor the performance of DAQDB  in different configurations suitable for High Energy Physics (HEP) data acquisition systems. It is largely based on the `minidaq` application. 

DAQDB-TOP is composed of two standalone software applications:

 - `producer`: application responsible for writing into the database
 - `consumer`: application responsible for reading from the database

The producer and consumer applications can be considered, respectively, as readout and filtering nodes in a typical HEP experiment. Based on these applications `run control` applications have been developed for the ATLAS trigger and data acquisition system.

## Quick test

Remove previously created pools from the persistent memory directory:
```sh
yes | rm /mnt/pmem/*.pm
```
Start the `producer` application:
```sh
[user@node bin]$ sudo ./producer --n-poolers 0 --base-core-id 5 --n-cores 29  --fragment-size 1000  --max-ready-keys 0 --n-dht-threads 1 --n-ro 1 --readout-cores 22  --number-events 1000000
```
Once the the writing application has finished, start the `consumer`:
```sh
[user@node bin]$ sudo sudo ./consumer   --n-poolers 0 --base-core-id 5 --n-cores 29   --n-dht-threads 1 --n-ff 1 --filtering-cores 56  --number-events 1000000
```
## Further details

The following list represents a subset of the most important configuration parameters to be controlled while testing. 

`--pmem-path STRING`

By default the `producer` application will created a persistent memory pool named "DAQDB_pool.pm" in the `mnt` directory.
```sh
# Example 
# Default: /mnt/pmem/DAQDB_pool.pm
--pmem-path /mnt/pmem/new_pool.pm
```
`--pmem-size NUMBER[bytes]`
By default the `producer` application will created a persistent memory pool with 15 GiB of size. 
```sh
# Example: create a pool of approximately 6 GiB 
--pmem-size 6005617664
```

`--number-events NUMBER`
This determines the total number of events to write or read. In case more threads are executed, each thread will process only a fraction of the total number of events.

`--n-dht-threads NUMBER` and `--base-core-id NUMBER`
These determine the number theads for the DHT and the starting core to which they are assigned.

`--n-cores NUMBER`
Total number of cores available for both DAQDB and producer/consumer worker threads. Just added as a safety system.

`--n-ro NUMBER`
Controls the number of `producer` threads .

`--readout-cores NUMBER`
Sets the starting core on which the `producer` threads will start.

`--n-ff NUMBER`
Controls the number of `consumer` threads.

`--filtering-cores NUMBER`
Sets the starting core on which the `consumer` threads will start.

`--fragment-size NUMBER[bytes]`
Sets the base fragments size of each entry in the database. By default it is set to 1024 bytes.

`--config-file STRING`
Define the configuration file for the DHT nodes. By default it is set to `server.cfg` for the producer application and `client.cfg` for the consumer application.

More configuration paramters can be found using `--help` command. Note that some of them are not implemented because are inherited from `minidaq` and kept for future developments. 

## Other considerations

Note that in both the `producer` and `consumer` application the key structure is set up such that the *componentID* is fixed and *eventID* is variable. This was kept for testing purposes only. 


## Future

 - Add monitoring system to visualize performance as a function of time
 - Automate testing procedure
 




