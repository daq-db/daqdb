# Data AcQuisition DataBase

* [Overview](#overview)
* [Installation](#installation)
	* [Source Code](#source-code)
	* [Prerequisites](#prerequisites)
	* [Build](#build)
* [Execution](#execution)
	* [Prerequisites](#prerequisites)
		* [SPDK](#spdk)
		* [Network](#network)
	* [Unit Tests](#unit-tests)
	* [Tools and Examples](#tools-and-examples)
		* [Minidaq benchmark](#minidaq-benchmark)
		* [CLI node example](#cli-node-example)

## Overview
DAQDB (Data Acquisition Database) — a distributed key-value store for high-bandwidth, generic data storage in event-driven systems.

DAQDB offers not only high-capacity and low-latency buffer for fast data selection, but also opens a new approach in high-bandwidth data acquisition by decoupling the lifetime of the data analysis processes from the changing event rate due to the duty cycle of the data source.

## Installation

#### Source Code
```
git clone https://github.com/daq-db/daqdb.git
cd ${daqdb_path}
git submodule update --init --recursive
```

#### Prerequisites

DaqDB must be build using CERN's LHC Computing Grid (LCG).

```
sudo yum install -y https://ecsft.cern.ch/dist/cvmfs/cvmfs-release/cvmfs-release-latest.noarch.rpm
sudo yum install cvmfs -y
sudo cvmfs_config setup
echo "CVMFS_REPOSITORIES=sft.cern.ch" | sudo tee -a /etc/cvmfs/default.local
echo "CVMFS_HTTP_PROXY=http://your-proxy.com:proxy-port | sudo tee -a /etc/cvmfs/default.local
cvmfs_config probe
ls /cvmfs/grid.cern.ch
```

#### Build


```
cd ${daqdb_path}
. /cvmfs/sft.cern.ch/lcg/views/setupViews.sh LCG_93 x86_64-centos7-gcc7-opt
cmake .
make -j$(nproc)
```
By default, all build products can be found in `${daqdb_path}/bin` folder.

```
make clean              # remove daqdb lib build files
make clean-dep          # remove third-party build files
make clean-all          # remove cmake, third-party and daqdb build files
make tests              # build tests
make test               # execute tests
```

_Note: <br>`. scripts/setup_env_lcg.sh` can be called to setup environment with LCG and SPDK._

_Note 2:<br> LCG_93 contains CMake 3.7 that may show warnings if BOOST library version is 1.64+._

## Execution

### Prerequisites

#### SPDK
DAQDB is using SPDK internally so following extra step is required to configure environment.

To be called once:
```
cd ${daqdb_path}
sudo third-party/spdk/scripts/pkgdep.sh
```

To be called each time:
```
cd ${daqdb_path}
sudo third-party/spdk/scripts/setup
```

#### Network

Mellanox ConnectX-4 or newer Ethernet NICs are supported at the moment.

Please see setup guide [here](doc/network_setup_guide.md)

### Unit Tests

```
cd ${daqdb_path}
cmake .
make tests -j$(nproc)
make test
```

### Tools and Examples

#### Minidaq benchmark

This application (located in apps/minidaq) is a simple benchmark that emulates
the operation of a typical Data AcQuisition (DAQ) system based on a KVS store.

Currently only a single node version using DAQDB library is supported. More details
are available in the built-in help of the application:
```
./minidaq --help
```

Tips for running minidaq:

* Administrative privileges are required. Run with root or sudo:
```
sudo LD_LIBRARY_PATH=`pwd`:$LD_LIBRARY_PATH ./minidaq --help
```
* LCG environment is required. Note:
```
. scripts/setup_env_lcg.sh
```
can be called to setup environment with LCG and SPDK.

* For single-node tests synchronous mode of operation for readout threads
is recommended.
* Time and persistent memory pool size should adjusted carefully. Depending on
the performance, memory pool can be too small for giving test and ramp times.
Performance can be degraded or interrupted.
* Persistent memory pool file should be deleted before each test run.
Unexpected behavior can occur otherwise.

#### CLI node example 
```
sudo ./cli-node -h
Options:
  -h [ --help ]                         Print help messages
  -p [ --port ] arg                     Node Communication port
  -d [ --dht ] arg                      DHT Communication port
  -n [ --nodeid ] arg (=0)              Node ID used to match database file
  -i [ --interactive ]                  Enable interactive mode
  -l [ --log ]                          Enable logging
  --pmem-path arg (=/mnt/pmem/pool.pm)  Rtree persistent memory pool file
  --pmem-size arg (=2147483648)         Rtree persistent memory pool size
  -c [ --spdk-conf-file ] arg (=../config.spdk)
                                        SPDK configuration file
```

To enter interactive mode execute cli-node with `--interactive` flag.
(Remember to allow writing to /mnt/pmem/ if not changing default --pmem-path)
```
sudo ./cli_node -i
daqdb> help
Following commands supported:
    - aget <key>
    - aput <key> <value> [-o <lock|ready|long_term> <0|1>]
    - aupdate <key> [value] [-o <lock|ready|long_term> <0|1>]
    - get <key>
    - help
    - node <id>
    - put <key> <value> [-o <lock|ready|long_term> <0|1>]
    - quit
    - remove <key>
    - status
    - update <key> [value] [-o <lock|ready|long_term> <0|1>]
```

#### Basic example

This application (located in examples/basic) provides examples how to
use DAQDB API (initialization, basic CRUD operations, ...).
