# FogKV

Contents
--------
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
Installation
------------

```
cd ${fogKvpath}
git submodule init
git submodule update
```

**Building**

following libraries are required:
<ul>
<li>boost-devel (1.60+)</li>
<li>boost-test (1.60+)</li>
<li>fabricpp-devel</li>
<li>log4cxx-devel (log4cxx-dev)</li>
<li>libpmemobj++</li>
</ul>

Invoke scons with the following parameters:

```
scons                 # build everything
scons -c              # remove build files
scons test            # execute unit tests
scons test verbose=1  # execute unit tests, prints all test messages
```
By default, all software can be found in ${fogKvpath}/bin folder.

<a name="execution"></a>
Execution
------------

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

**Interactive mode**

To enter interactive mode execute dragon with `--interactive` flag.

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

