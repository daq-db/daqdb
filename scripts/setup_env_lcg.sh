#!/bin/sh

pushd . > /dev/null
SCRIPT_PATH="${BASH_SOURCE[0]}";
if ([ -h "${SCRIPT_PATH}" ]) then
  while([ -h "${SCRIPT_PATH}" ]) do cd `dirname "$SCRIPT_PATH"`; SCRIPT_PATH=`readlink "${SCRIPT_PATH}"`; done
fi
cd `dirname ${SCRIPT_PATH}` > /dev/null
SCRIPT_PATH=`pwd`;
popd  > /dev/null

SPDKPATH=third-party/spdk

cvmfs_config probe

source /cvmfs/sft.cern.ch/lcg/views/setupViews.sh LCG_95 x86_64-centos7-gcc7-opt

# sRPC requires at least 1024 of 2M hugepages (2048 for 2 sockets platform)
sudo HUGEMEM=2048 $SCRIPT_PATH/../$SPDKPATH/scripts/setup.sh
