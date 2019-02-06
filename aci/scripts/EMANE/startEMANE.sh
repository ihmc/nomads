#!/bin/bash

cd /home/nomads

# Setup VLANs
./setupVLANs.sh

# Kill any active screen session
#screen -ls | tail -n +2 | head -n -1 | cut -d. -f1 | awk '{print $1}' | xargs kill -9 &> /dev/null
pkill -9 -f 'SCREEN' &> /dev/null
screen -wipe &> /dev/null

# Start EMANE
screen -dmS emane -s /bin/bash
screen -r emane -X screen emane -r /home/nomads/emanefiles/commEffect/platform8.xml

sleep 3

# Start the transport daemon
screen -dmS transportd -s /bin/bash
screen -r transportd -X screen emanetransportd -r /home/nomads/emanefiles/commEffect/transportdaemon8.xml


exit 0
