#!/bin/bash

ENIS=eth1,eth2,eth4,eth5
INIS=eth0

#ntpdate 192.168.1.254

#./set_routes.sh $ENI $INI

# Kill any active screen session
screen -ls | tail -n +2 | head -n -1 | cut -d. -f1 | awk '{print $1}' | xargs kill &> /dev/null
screen -wipe

# Start tcpdump on all network interfaces
./start_tcpdump.sh $ENIS $INIS

# Start NetProxy in a dedicated screen session
screen -dmS netproxy -s /bin/bash
screen -r netproxy -X screen ./IHMC/bin/netproxy.sh $ENIS $INIS

exit 0
