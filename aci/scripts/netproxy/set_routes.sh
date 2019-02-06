#!/bin/bash

if test $# -lt 2
then
	echo "Wrong argument number! Correct usage is: $0 <EXT_IFACE_NAME> <INT_IFACE_NAME>"
	exit 1
fi

ENI=$1
shift
INI=$1
shift

sudo ip route del 192.168.1.0/24 dev $ENI
sudo ip route add 192.168.1.254/32 dev $ENI
