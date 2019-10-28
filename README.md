# Data AcQuisition DataBase

- [Data AcQuisition DataBase](#data-acquisition-database)
  - [Overview](#overview)
  - [Installation](#installation)
      - [Source Code](#source-code)
      - [Prerequisites](#prerequisites)
      - [Build](#build)
  - [Execution](#execution)
    - [Initial Steps](#initial-steps)
      - [SPDK](#spdk)
      - [Network](#network)
    - [Unit Tests](#unit-tests)
    - [Tools and Examples](#tools-and-examples)
      - [Minidaq benchmark](#minidaq-benchmark)
      - [CLI node example](#cli-node-example)
      - [Basic example](#basic-example)

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

---

#### Prerequisites

* asio-devel (1.10+)
* autoconf (2.69+)
* boost-devel (1.64+)
* boost-test (1.64+)
* cmake (3.11+)
* gtest-devel (1.8+)
* nasm (2.13+) (http://www.nasm.us)

##### CERN's LHC

DaqDB could be built using CERN's LHC Computing Grid (LCG).

```
sudo yum install -y https://ecsft.cern.ch/dist/cvmfs/cvmfs-release/cvmfs-release-latest.noarch.rpm
sudo yum install cvmfs -y
sudo cvmfs_config setup
echo "CVMFS_REPOSITORIES=sft.cern.ch" | sudo tee -a /etc/cvmfs/default.local
echo "CVMFS_HTTP_PROXY=http://your-proxy.com:proxy-port | sudo tee -a /etc/cvmfs/default.local
cvmfs_config probe
ls /cvmfs/grid.cern.ch
```
##### Fedora 28

The dependencies can be installed automatically by `scripts/pkgdep.sh`.
```
cd ${daqdb_path}
sudo ./scripts/pkgdep.sh
sudo third-party/spdk/scripts/pkgdep.sh
``` 

_Note: <br> No support for Mellanox cards on Fedora 29 at the moment._

---

#### Build


```
cd ${daqdb_path}
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

## Execution

#### Initial Steps

##### SPDK
DAQDB is using SPDK internally so following extra step is required to configure environment.

To be called once:
```
cd ${daqdb_path}
sudo third-party/spdk/scripts/pkgdep.sh
```

To be called each time:
```
cd ${daqdb_path}
sudo HUGEMEM=4096 third-party/spdk/scripts/setup
```
_Note: <br> eRPC requires at least 4096 of 2M hugepages_

##### Network

Mellanox ConnectX-4 or newer Ethernet NICs are supported at the moment.

Please see setup guide [here](doc/network_setup_guide.md)

---

#### Unit Tests

```
cd ${daqdb_path}
cmake .
make tests -j$(nproc)
make test
```

---

#### Tools and Examples

##### Minidaq benchmark

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

##### CLI node example 
```
sudo ./clinode -h
Options:
  -h [ --help ]                         Print help messages
  -i [ --interactive ]                  Enable interactive mode
  -l [ --log ]                          Enable logging
  -c [ --config-file ] arg (=clinode.cfg)
                                        Configuration file
```

To enter interactive mode execute cli-node with `--interactive` flag.
(Remember to allow writing to /mnt/pmem/ if not changing default --pmem-path)
```
sudo ./cli_node -i
daqdb> help
Following commands supported:
	- aget <key> [-o <long_term> <0|1>]
	- aput <key> <value> [-o <lock|ready|long_term> <0|1>]
	- aupdate <key> [value] [-o <lock|ready|long_term> <0|1>]
	- get <key> [-o <long_term> <0|1>]
	- help
	- neighbors
	- put <key> <value> [-o <lock|ready|long_term> <0|1>]
	- quit
	- remove <key>
	- status
	- update <key> [value] [-o <lock|ready|long_term> <0|1>]
```

##### Basic example

This application (located in examples/basic) provides examples how to
use DAQDB API (initialization, basic CRUD operations, ...).
