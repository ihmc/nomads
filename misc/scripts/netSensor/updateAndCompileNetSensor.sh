#!/bin/bash

cd ../../cpp
cvs update -d
cd netsensor/linux
make
cd ../../../scripts/netSensor