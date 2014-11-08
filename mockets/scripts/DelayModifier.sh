#!/bin/bash

# This file can be used from a machine with NistNet installed
# to change the delay from two nodes.
# This file needs the executable timeScreenPrinter and the java class Sleeper in the same directory:
# timeScreenPrinter can be found in nomads/aci/cpp/timeEval/linux
# Sleeper can be found in nomads/mockets/scripts.
# Created for test Mockets RTT estimation.

UPPERBOUND=40
LOWERBOUND=20
if [ "$#" -lt "3" ]; then
    echo "Usage ./DelayModifier.sh <SourceIP> <DestIP> <Milliseconds to sleep>"
    exit 2
fi
echo "Start the process..."
# Start NistNet
cnistnet -u
echo $1
echo $2

{
# First loop from 20 to 40 ms
for ((a=$LOWERBOUND; a<=$UPPERBOUND; a++))
do
    # System time in milliseconds
    TIMESTAMP=`./timeScreenPrinter`
    # Set the new delay $a for packets from machine $1 to machine $2
    cnistnet -a $1 $2 --delay $a
    # System time in milliseconds
    #TIMESTAMP=`./timeScreenPrinter`
    # Print Delay and Timestamp
    echo "Delay $a, Timestamp $TIMESTAMP"
    # Call a java file to perform a sleep of $3 milliseconds
    java Sleeper $3
done
echo
# Second loop from 39 to 20 ms
for ((a=$(($UPPERBOUND-1)); a>=LOWERBOUND; a--))
do
    # System time in milliseconds
    TIMESTAMP=`./timeScreenPrinter`
    # Set the new delay $a for packets from machine $1 to machine $2 
    cnistnet -a $1 $2 --delay $a
    # System time in milliseconds
    #TIMESTAMP=`./timeScreenPrinter`
    # Print Delay and Timestamp
    echo "Delay $a, Timestamp $TIMESTAMP"
    # Call a java file to perform a sleep of $3 milliseconds
    java Sleeper $3
done
} > "RTTTestResultDelay.txt"

echo "DelayModifier finished. Results are in RTTTestResultDelay.txt."
echo

exit 0
