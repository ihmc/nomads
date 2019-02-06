#!/bin/bash

if test $# -lt 2
then
	echo "Wrong argument number! Correct usage is: $0 <EXT_IFACES_LIST> <INT_IFACES_LIST>"
	echo "<EXT_IFACES_LIST> and <INT_IFACES_LIST> are a comma-separated list of names of network interfaces with no spaces"
	exit 1
fi

NODE_ID='TOC'
PREFIX=`date +%Y-%m-%d-%H:%M:%S`
ENIS=$1
shift
INIS=$1
shift

# Start tcpdump on all network interfaces
for ENI in $(echo $ENIS | sed "s/,/ /g")
do
	screen -dmS tcpdump_$ENI -s /bin/bash
	screen -r tcpdump_$ENI -X screen tcpdump -i $ENI -w /home/nomads/IHMC/traces/${NODE_ID}.${PREFIX}.${ENI}.pcap -U -C 512 -z gzip
done

for INI in $(echo $INIS | sed "s/,/ /g")
do
	screen -dmS tcpdump_$INI -s /bin/bash
	screen -r tcpdump_$INI -X screen tcpdump -i $INI -w /home/nomads/IHMC/traces/${NODE_ID}.${PREFIX}.${INI}.pcap -U -C 512 -z gzip
done


exit 0
