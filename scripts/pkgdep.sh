#!/bin/sh
# Please run this script as root.

SYSTEM=`uname -s`

if [ -s /etc/centos-release ]; then
	yum install cmake
	yum install boost boost-test boost-devel
	yum install asio-devel
	yum install jsoncpp jsoncpp-devel
elif [ -s /etc/redhat-release ]; then
    # Includes Fedora
    dnf install cmake
    dnf install boost boost-test boost-devel
    dnf install asio-devel
    dnf install jsoncpp jsoncpp-devel
elif [ -f /etc/debian_version ]; then
    # Includes Ubuntu, Debian
    apt-get install cmake
    apt-get install libboost-all-dev
    apt-get install libasio-dev
    apt-get install libjsoncpp1 libjsoncpp-dev 
else
	echo "pkgdep: unknown system type."
	exit 1
fi
