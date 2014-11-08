#!/bin/bash

# This file can be used from a machine with NistNet installed
# to change the delay from a second node.
# This file needs the executable timeScreenPrinter and the java class Sleeper in the same directory:
# timeScreenPrinter can be found in nomads/aci/cpp/timeEval/linux
# Sleeper can be found in nomads/mockets/scripts.
# Created for test Mockets RTT estimation.

echo "Start the process..."

if [ "$#" -lt "1" ]; then
    echo Usage ./DelayModifier.sh <SecondNodeInvolvedIP>
    exit 2
fi
# Start NistNet
cnistnet -u
cnistnet -a $1 0.0.0.0 --delay 0
# Start the client ***adjust the path***
./IHMC/work/nomads/mockets/test/cpp/linux/RTTClientServerTest -client -remotehost $1 -iterations 65000

{
# Change the delay from 0 to n*100 ms
for $a in "0" "100" "0" "200" "0" "300" "0" "400" "0" "500" "0"
do
    # Set the new delay $a for packets from machine $1
    cnistnet -a $1 0.0.0.0 --delay $a
    # System time in milliseconds
    TIMESTAMP=`./timeScreenPrinter`
    # Print Delay and Timestamp
    echo "Delay $a, Timestamp $TIMESTAMP"
    # Call a java file to perform a sleep of 1000 milliseconds
    java Sleeper 1000
done
} > "RTTTestResultDelay.txt"

echo "DelayModifier finished. Results are in RTTTestResultDelay.txt."
echo

exit 0
