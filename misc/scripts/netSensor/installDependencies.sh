#!/bin/bash

sudo apt-get remove libprotobuf-dev
sudo apt-get install libpcap-dev
cd ../../../externals/protobuf/
sudo sh installProtobuf.sh
cd ../../misc/scripts/netSensor
make clean -C ../../cpp/netsensor/linux
make -C ../../cpp/netsensor/linux