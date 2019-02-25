#!/bin/sh
# Please run this script as root.

SYSTEM=`uname -s`

if [ -s /etc/redhat-release -o -s /etc/centos-release ]; then
    # Includes Fedora
    yum install cmake boost boost-test boost-devel asio-devel autoconf gtest gtest-devel -y
else
	echo "pkgdep: unknown system type."
	exit 1
fi
