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
cd ${fogKvpath}
git submodule init
git submodule update
cd src/3rd/cChord
git checkout master
git pull
```

**Building**

following libraries required:
<ul>
<li>boost</li>
<li>boost-devel</li>
<li>boost-test</li>
</ul>

Invoke scons with the following parameters:

```
scons                 # build everything
scons -c              # remove build files
```
By default, all software can be found in ${fogKvpath}/build folder.