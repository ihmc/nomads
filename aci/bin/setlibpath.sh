#!/bin/bash

export NOMADS_HOME=../..

if test -z $EXTERNALS_DIR
then
    export EXTERNALS_DIR=$NOMADS_HOME/externals/lib
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$EXTERNALS_DIR/linux-x86
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/../jre/lib/i386/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/../jre/lib/i386/client
