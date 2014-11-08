#!/bin/bash

java -Djava.library.path=../../../../bin -cp ../../../../lib/mocket.jar:$NOMADS_HOME/util/lib/util.jar:../../../../build/antcache  us.ihmc.mockets.test.FileRecv $1 $2 $3

