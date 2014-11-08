#!/bin/bash

# Configuration script for the TUN/TAP interface
# Launch the script specifying the following parameters:
#       -IPAddr <IP_ADDR>
#       -subnetmask <SUBNET_MASK>
#       -user <USER_NAME>
#

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

echo "parameters:" $*

if [ $# -ne 6 ]
then
    echo "Correct usage (without < and > charachters):"
    echo "./setupTap0.sh -IPAddr <IP_ADDR> -subnetmask <SUBNET_MASK> -user <USER_NAME>"
    exit 1
fi

while [ $# -ge 2 ]
do
    case $1 in
        "-IPAddr"       )
                            shift
                            if ! (valid_ip "$1")
                                then
                                    echo "Wrong IP value: '$1'. IP must have format A.B.C.D"
                                    exit 3
                                else
                                    OIFS=$IFS; IFS='.'; IP=($1); IFS=$OIFS; IP_STRING=$1;
                                    shift
                            fi;;
                    
        "-subnetmask"   )
                            shift
                            if ! (valid_ip "$1")
                                then
                                    echo "Wrong subnetmask: '$1'. Mask must have the format A.B.C.D"
                                    exit 4
                                else
                                    SNMASK_STRING=$1
                                    shift
                            fi;;
        "-user"         )
                            shift; USER=$1; shift;;
        *               )
                            echo "Correct usage (without < and > charachters):"
                            echo "./setupTap0.sh -IPAddr <IP_ADDR> -subnetmask <SUBNET_MASK>" \
                                    "-user <USER_NAME>"
                            exit 2;;
    esac    
done

if [ ! "$IP_STRING" ]
then
    echo "IP Address not specified"; exit 5;
fi
#echo "Using IP Address:" $IP_STRING

if [ ! "$SNMASK_STRING" ]
then
    echo "subnet mask not specified"; exit 6;
fi
#echo "Set subnetmask to value" $SNMASK_STRING

if [ ! "$USER" ]
then
    echo "user not specified"; exit 7;
fi
#echo "User is" $USER

MAC1=`echo "obase=16; ${IP[2]}" | bc`
MAC2=`echo "obase=16; ${IP[3]}" | bc`

sudo /usr/sbin/tunctl -d tap0 > null
sudo /usr/sbin/tunctl -u $USER -t tap0
sudo /sbin/ifconfig tap0 down
sudo /sbin/ifconfig tap0 hw ether 02:0a:0c:00:$MAC1:$MAC2
sudo /sbin/ifconfig tap0 "$IP_STRING" netmask $SNMASK_STRING
sudo /sbin/ifconfig tap0 up



##############################################################
# CODE TO LOCATE COMMANDS WHICH ARE NOT IN THE STANDARD PATH #
##############################################################

#TUNCTL_PATH=`locate tunctl | sed -n 1p`
#
#if [[ $TUNCTL_PATH ]]
#then
#    echo "Found tunctl in path" $TUNCTL_PATH
#    
#else
#    echo "Impossible to locate tunctl command"
#    echo "Please, be sure that the uml-utilities package is correctly installed" \
#            "and updatedb command has been launched"
#fi

#IFCONFIG_PATH=`locate ifconfig | sed -n 1p`

#if [[ $IFCONFIG_PATH ]]
#then
#    echo "Found ifconfig in path" $IFCONFIG_PATH
#    
#else
#    echo "Impossible to locate ifconfig command"
#    echo "Please, be sure that the net-tools package is correctly installed" \
#            "and updatedb command has been launched"
#fi

#sudo "$TUNCTL_PATH" -u $USER -t tap0
#sudo "$TUNCTL_PATH" -d tap0
#sudo "$IFCONFIG_PATH" tap0 down
#sudo "$IFCONFIG_PATH" tap0 hw ether 02:0a:0c:00:${IP[2]}:${IP[3]}
#sudo "$IFCONFIG_PATH" tap0 "$IP_STRING" netmask 255.255.255.0
#sudo "$IFCONFIG_PATH" tap0 up


sudo /bin/chown $USER:$USER /dev/net/tun
#sudo /usr/sbin/tunctl -d tap0

