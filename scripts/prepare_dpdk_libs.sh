#!/bin/sh
set -x
SCRIPT=$(readlink -f $0)
SCRIPTPATH=`dirname $SCRIPT`
DPDKPATH=third-party/dpdk

cd $SCRIPTPATH/../$DPDKPATH
rm -f x86_64-native-linuxapp-gcc/lib/libdpdk.a
ar -M <$SCRIPTPATH/libdpdk.mri
cd -
set +x
