# FogKV

## Contents

<ul>
<li><a href="#overview">Overview</a></li>
<li><a href="#installation">Installation</a></li>
<li><a href="#execution">Execution</a></li>
</ul>

<a name="overview"></a>
Overview
--------
Scalable distributed, low-latency key/value store with range queries.

<a name="installation"></a>
## Installation

#### Source Code
```
git clone https://github.com/FogKV/FogKV.git
cd ${fogKVpath}
git submodule update --init --recursive
```

#### Prerequisites

The dependencies can be installed automatically by scripts/pkgdep.sh.
```
cd ${fogKVpath}
scripts/pkgdep.sh
third-party/spdk/scripts/pkgdep.sh
```

Following libraries are required:
<ul>
<li>asio-devel</li>
<li>libjsoncpp</li>
</ul>
Following libraries are optional (needed for examples and unit tests):
<ul>
<li>boost-devel (1.63+)</li>
<li>boost-test (1.63+)</li>
<li>doxygen</li>
</ul>

##### LCG

Another option is building against LHC Computing Grid (LCG) release. Steps to configure the LCG environemt:
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
cd ${fogKVpath}
cmake .
make -j$(nproc)
```
By default, all software can be found in ${fogKvpath}/bin folder.

```
make clean              # remove fogkv lib build files
make clean-dep          # remove third-party build files
make clean-all          # remove cmake, third-party and fogkv build files
```

##### LCG
If using LCG release, set the desired environment first:
```
. /cvmfs/sft.cern.ch/lcg/views/setupViews.sh LCG_93 x86_64-centos7-gcc7-opt
```
or 
```
. /cvmfs/sft.cern.ch/lcg/views/setupViews.sh LCG_latest x86_64-centos7-gcc7-opt
```

Note: `. scripts/setup_env_lcg.sh` can be called to setup environment with LCG and SPDK.
Note2: LCG_93 contains CMake 3.7 that may show warnings if BOOST library version is 1.64+.

##### SPDK
FogKV is using SPDK internally so following extra step is required to configure environment.

To be called once:
```
cd ${fogKVpath}
sudo third-party/spdk/scripts/pkgdep.sh
```

To be called each time:
```
cd ${fogKVpath}
sudo third-party/spdk/scripts/setup
```

If using LCG then execute as root with LCG initialized or remember to preserve user LD_LIBRARY_PATH in visudo.
Example:

```
cd ${fogKVpath}/bin
sudo LD_LIBRARY_PATH="$(pwd):$LD_LIBRARY_PATH" ./clinode -i
```

#### Unit Tests

```
cd ${fogKVpath}
cmake .
make -j$(nproc)
ctest
```

<a name="execution"></a>
## Execution

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
fogkv> help
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
use FogKV API (initialization, basic CRUD operations, ...).

#### Minidaq benchmark

This application (located in apps/minidaq) is a simple benchmark that emulates
the operation of a typical Data AcQuisition (DAQ) system based on a KVS store.

Currently only a single node version using FogKV library is supported. More details
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
* Recommended environment variables for PMDK: 
```
export PMEM_IS_PMEM_FORCE=1
export PMEM_NO_FLUSH=1
export PMEMOBJ_CONF="prefault.at_open=1;prefault.at_create=1"
```
* For single-node tests synchronous mode of operation for readout threads
is recommended.
* Time and persistent memory pool size should adjusted carefully. Depending on
the performance, memory pool can be too small for giving test and ramp times.
Performance can be degraded or interrupted.
* Persistent memory pool file should be deleted before each test run.
Unexpected behavior can occur otherwise.
