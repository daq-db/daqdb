#!/bin/sh
# Please run this script as root.

SYSTEM=`uname -s`

if [ -s /etc/centos-release ]; then
	yum install -y scons

elif [ -s /etc/redhat-release ]; then
    # Includes Fedora

    # FogKV Building environment dependencies
    dnf install scons cmake
    # Boost dependencies
    dnf install -y boost boost-test boost-devel
    # logging dependencies
    dnf install -y log4cxx log4cxx-devel
    # logging dependencies
    dnf install -y jsoncpp jsoncpp-devel

elif [ -f /etc/debian_version ]; then
    # Includes Ubuntu, Debian
	
    # FogKV Building environment dependencies
    apt-get install -y scons cmake
    # Boost dependencies
    apt-get install -y libboost1.63-all-dev
    # logging dependencies
    apt-get install -y liblog4cxx10v5 liblog4cxx-dev
    # jsoncpp dependencies
    libjsoncpp libjsoncpp-dev 

else
	echo "pkgdep: unknown system type."
	exit 1
fi
