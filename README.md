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
./scripts/pkgdep.sh
```
Following libraries are required:
<ul>
<li>boost-devel (1.63+)</li>
<li>boost-test (1.63+)</li>
<li>libjsoncpp</li>
</ul>
Following libraries are optional:
<ul>
<li>log4cxx-devel (log4cxx-dev)</li>
</ul>

##### LCG

Another option is building against LHC Computing Grid (LCG) release. Steps to configure the LCG environemt:
```
sudo yum install -y https://ecsft.cern.ch/dist/cvmfs/cvmfs-release/cvmfs-release-latest.noarch.rpm
sudo cvmfs_config setup
echo "CVMFS_REPOSITORIES=sft.cern.ch" | sudo tee -a /etc/cvmfs/default.local
echo "CVMFS_HTTP_PROXY=http://your-proxy.com:proxy-port | sudo tee -a /etc/cvmfs/default.local
cvmfs_config probe
ls /cvmfs/grid.cern.ch

```

#### Build

Invoke scons with the following parameters:

```
scons                 # build everything
scons -c              # remove build files
scons --lcg           # build against CERN LCG
```
By default, all software can be found in ${fogKvpath}/bin folder.

##### LCG
If using LCG release, set the desired environemt:
```
. /cvmfs/sft.cern.ch/lcg/views/setupViews.sh LCG_89 x86_64-centos7-gcc7-opt
```

#### Unit Tests

Invoke scons with the following parameters:
```
scons test            # execute unit tests
scons test verbose=1  # execute unit tests, prints all test messages
```

<a name="execution"></a>
## Execution

#### CLI node example 
```
./cli-node -h
Options:
  -h [ --help ]            Print help messages
  -p [ --port ] arg        Node Communication port
  -d [ --dht ] arg         DHT Communication port
  -n [ --nodeid ] arg (=0) Node ID used to match database file. If not set DB 
                           file will be removed when node stopped.
  -i [ --interactive ]     Enable interactive mode
  -l [ --log ]             Enable logging
```

To enter interactive mode execute cli-node with `--interactive` flag.

```
./cli_node -i
DHT node (id=107) is running on 127.0.0.1:11000
Waiting for requests on port 40401. Press CTRL-C to exit.
fogkv> help
Following commands supported:
	- get <key>
	- help
	- put <key> <value>
	- quit
	- remove <key>
	- status

```

