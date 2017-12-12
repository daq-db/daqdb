#!/bin/sh
# Please run this script as root.

SYSTEM=`uname -s`

if [ -s /etc/redhat-release ]; then
	# Includes Fedora

	# FogKV Building environment dependencies
	dnf install scons
	# Boost dependencies
	dnf install -y boost boost-test boost-devel
	# NVML library
	dnf install -y libpmem libpmemobj++-devel
	# RDMA related dependencies
	dnf install -y libfabric libfabric-devel
	# logging dependencies
	dnf install -y log4cxx log4cxx-devel

elif [ -f /etc/debian_version ]; then
	# Includes Ubuntu, Debian
	# TODO
	echo "pkgdep: not implemented yet."
 
else
	echo "pkgdep: unknown system type."
	exit 1
fi
