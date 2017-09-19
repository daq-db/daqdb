# FogKV

Contents
--------
<ul>
<li><a href="#overview">Overview</a></li>
<li><a href="#installation">Installation</a></li>
</ul>

<a name="overview"></a>
Overview
--------

<a name="installation"></a>
Installation
------------

```
cd 3rd/cChord
git submodule init
git submodule update
git checkout master
git pull
```

**Building with make**

You'll need `make` and `g++` installed.
following libraries required:
<ul>
<li>boost</li>
<li>boost-devel</li>
<li>boost-test</li>
</ul>


```
make                  # build everything
make clean            # remove build files
make test             # build and run unit tests
```
