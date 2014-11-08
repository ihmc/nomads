#!/bin/bash

#
# Configuration script for the ACM NetProxy application
# Launch the script specifying the following parameters:
#       -IPAddr <IP_ADDR>
#       -subnetmask <SUBNET_MASK>   [OPTIONAL]
#       -user <USER_NAME>           [OPTIONAL]
#
# First, the script launches the setupTap0.sh script, passing all the parameters.
# Then, the script looks for the netproxy.cfg config file to configure it
#   The file must be located in the directory ../conf
#


NETPROXY_CONFIG_FILE="../conf/netproxy.cfg"
if [ ! -e "$NETPROXY_CONFIG_FILE" ]
then
    echo "Error:" $NETPROXY_CONFIG_FILE "file not found!"
    exit 1
fi

valid_ip()
{
    local ip=$1
    local stat=1

    if [[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]
    then
        OIFS=$IFS
        IFS='.'
        ip=($ip)
        IFS=$OIFS
        [[ ${ip[0]} -le 255 && ${ip[1]} -le 255 && ${ip[2]} -le 255 && ${ip[3]} -le 255 ]]
        stat=$?
    fi
    return $stat
}


if [[ $# -ne 2 && $# -ne 4 && $# -ne 6 ]]
then
    echo "Correct usage (without < and > charachters):"
    echo "./configureNetProxy.sh -IPAddr <IP_ADDR> [-subnetmask <SUBNET_MASK>] [-user <USER_NAME>]"
    exit 2
fi

while [ $# -ge 2 ]
do
    case $1 in
        "-IPAddr"       )
                            shift
                            if ! (valid_ip "$1")
                                then
                                    echo "Wrong IP value: '$1'. IP must have format A.B.C.D"
                                    exit 4
                                else
                                    OIFS=$IFS; IFS='.'; IP=($1); IFS=$OIFS; IP_STRING=$1;
                                    shift
                            fi;;
                    
        "-subnetmask"   )
                            shift
                            if ! (valid_ip "$1")
                                then
                                    echo "Wrong subnetmask: '$1'. Mask must have the format A.B.C.D"
                                    exit 5
                                else
                                    SNMASK_STRING=$1
                                    shift
                            fi;;
        "-user"         )
                            shift; USER=$1; shift;;
        *               )
                            echo "Correct usage (without < and > charachters):"
                            echo "./setupTap0.sh -IPAddr <IP_ADDR> [-subnetmask <SUBNET_MASK>]" \
                                    "[-user <USER_NAME>]"
                            exit 3;;
    esac    
done

if [ ! "$IP_STRING" ]
then
    echo "IP Address not specified"; exit 6;
fi
echo "Using IP Address:" $IP_STRING
CONFIGURED_IPADDRESS_STRING="IPAddress=$IP_STRING"

if [ ! "$SNMASK_STRING" ]
then
    SNMASK_STRING="255.255.255.0"
fi
echo "Set subnetmask to value" $SNMASK_STRING

if [ ! "$USER" ]
then
    USER=`whoami`
fi
echo "User is" $USER

./setupTap0.sh "-IPAddr" $IP_STRING "-subnetmask" $SNMASK_STRING "-user" $USER
if [ $? -ne 0 ]
then
    echo "Error occurred installing TUN/TAP Virtual Interface"
    exit 7
fi

PATTERN2MATCH="IPAddress=[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}"
LINE2SUBSTITUTE=`grep -E -m 1 -n -e "$PATTERN2MATCH" "$NETPROXY_CONFIG_FILE"`
if [ ! $LINE2SUBSTITUTE ]
then
    echo "Error: IPAddress entry not found in the config file"
    exit 8
fi

LINENUMBERLIMIT=`expr index "$LINE2SUBSTITUTE" ":"`
LINENUMBER=${LINE2SUBSTITUTE:0:$(($LINENUMBERLIMIT - 1))}
sed -i -e "$LINENUMBER c\\$CONFIGURED_IPADDRESS_STRING" $NETPROXY_CONFIG_FILE

