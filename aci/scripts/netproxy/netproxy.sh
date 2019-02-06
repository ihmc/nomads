#!/bin/bash

if test $# -lt 2
then
	echo "Wrong argument number! Correct usage is: $0 <EXT_IFACES_LIST> <INT_IFACES_LIST>"
	echo "<EXT_IFACES_LIST> and <INT_IFACES_LIST> are a comma-separated list of names of network interfaces with no spaces"
	exit 1
fi


ENIS=$1
shift
INIS=$1
shift

# lro, gro, and gso off for all network interfaces
for ENI in $(echo $ENIS | sed "s/,/ /g")
do
    ethtool -K $ENI lro off gro off gso off
done

for INI in $(echo $INIS | sed "s/,/ /g")
do
	ethtool -K $INI lro off gro off gso off
done

# Copy Mockets configuration file
cp /home/nomads/IHMC/conf/mockets/mockets.conf /tmp

# Run NetProxy within GDB
cd /home/nomads/IHMC/aci/bin
gdb -ex run --args netProxy -conf /home/nomads/IHMC/conf/netproxy/netproxy.cfg


exit 0
