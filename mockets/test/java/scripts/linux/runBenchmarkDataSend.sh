#!/bin/bash

java -Djava.library.path=../../../../bin -cp ../../../../lib/mocket.jar:$NOMADS_HOME/util/lib/util.jar:../../../../build/antcache  us.ihmc.mockets.test.BenchmarkThroughputDataSend $1 $2 $3 $4 $5

