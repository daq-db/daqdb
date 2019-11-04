#!/bin/sh

SCRIPT=$(readlink -f $0)
SCRIPTPATH=`dirname $SCRIPT`

cp $SCRIPTPATH/autogen_isal.sh $SCRIPTPATH/../third-party/spdk/isa-l/autogen.sh
