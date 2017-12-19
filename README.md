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
following libraries are required:
<ul>
<li>boost-devel (1.63+)</li>
<li>boost-test (1.63+)</li>
<li>fabricpp-devel</li>
<li>log4cxx-devel (log4cxx-dev)</li>
<li>libpmemobj++</li>
</ul>

#### Build

Invoke scons with the following parameters:

```
scons                 # build everything
scons -c              # remove build files
```
By default, all software can be found in ${fogKvpath}/bin folder.

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

