#!/bin/sh
set -e

SCRIPT=$(readlink -f $0)
SCRIPTPATH=`dirname $SCRIPT`
SPDKPATH=third-party/spdk

cd $SCRIPTPATH/../$SPDKPATH
ar -M <$SCRIPTPATH/libspdk.mri
cd -
