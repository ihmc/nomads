#ifndef NETSENSOR_NetSensorConstants__INCLUDED
#define NETSENSOR_NetSensorConstants__INCLUDED
/*
* NetSensorConstants.h
* Author: rfronteddu@ihmc.us bordway@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 3 (GPLv3) as published by the Free Software Foundation.
*
* U.S. Government agencies and organizations may redistribute
* and/or modify this program under terms equivalent to
* "Government Purpose Rights" as defined by DFARS
* 252.227-7014(a)(12) (February 2014).
*
* Alternative licenses that allow for use within commercial products may be
* available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
*
* This file holds all the NetSensor constants values
*
*/
#include "FTypes.h"
#include "InetAddr.h"
namespace IHMC_NETSENSOR
{
const uint32 C_VERSION = 2;
const uint32 C_PACKET_MAX_SIZE = 1400;

// Configuration File
const char *const  C_VERSION_CONF_KEY               = "netsensor.version";
const char *const  C_DELIVERY_IPS_CONF_KEY          = "netsensor.delivery.ips";
const char *const  C_MONITORED_INTERFACES_CONF_KEY  = "netsensor.monitored.interfaces";
const char *const  C_DELIVERY_PORT_KEY              = "netsensor.delivery.port";
const char *const  C_DELIVERY_MTU_KEY               = "netsensor.delivery.mtu";
const char *const  C_DELIVERY_TRAFFIC_PERIOD_KEY    = "netsensor.traffic.delivery.time";
const char *const  C_DELIVERY_TOPOLOGY_PERIOD_KEY   = "netsensor.topology.delivery.time";
const char *const  C_LAX_CONF_KEY                   = "netsensor.topology.lax.active";
const char *const  C_ARP_CONF_KEY                   = "netsensor.topology.arp.active";
const char *const  C_NETMASK_CONF_KEY               = "netsensor.topology.netmask.active";
const char *const  C_NETPROXY_CONF_KEY              = "netsensor.topology.netproxy.active";
const char *const  C_EXTERNAL_TOP_CONF_KEY          = "netsensor.topology.externals.active";
const char *const  C_IGNORE_MULTICAST_CONF_KEY      = "netsensor.traffc.ignore.multicast";
const char *const  C_ICMP_CONF_KEY                  = "netsensor.rtt.detection.icmp.active";

const char *const  C_EXTERNAL_TOPOLOGY_UNKNOWN = "Unknown";

const int C_TRAFFIC_CLEANING_PERIOD = 3500;
const int C_TOPOLOGY_CLEANING_PERIOD = 3500;
const int C_RTT_CLEANING_PERIOD = 3500;

const uint32 C_MULTICAST_MAX_ADDRESS = NOMADSUtil::InetAddr("255.255.255.239").getIPAddress();
const uint32 C_MULTICAST_MIN_ADDRESS = NOMADSUtil::InetAddr("0.0.0.224").getIPAddress();

const uint32 C_MAX_CLEANING_NUMBER = 10000;
const uint32 C_ENTRY_TIME_VALIDITY = 5000;

const int64 C_CLEANING_TIME = 300;
const int64 C_SENDING_TIME = 4000;
const int64 C_SLEEP_TIME = 1000;

//Debug Timing
const int64 C_D_TRAFFIC_HANDLING_TIME   = 50;
const int64 C_D_TOPOLOGY_HANDLING_TIME  = 50;
const int64 C_D_TRAFFIC_PROTO_TIME      = 50;
const int64 C_D_TOPOLOGY_PROTO_TIME     = 50;


//DEBUG
const bool C_CLASSIFICATION_DEBUG_MODE = false;

//Command line parameters
const char* const C_REPLAY_MODE                 = "-rm";
const char* const C_USE_COMPRESSION             = "-cp";
const char* const C_CALCULATE_TCP_RTT           = "-trtt";
const char* const C_STORE_EXTERNALS_TOPOLOGY    = "-et";
const char* const C_SPECIFY_INTERFACE           = "-i";
const char* const C_SPECIFIY_CONFIG_PATH        = "-conf";
const char* const C_SPECIFY_NS_RECIPIENT        = "-nr";
const char* const C_SHOW_HELP                   = "-h";
const uint8 C_INT_PARAM_UNKNOWN                 = -1;
const uint8 C_INT_SHOW_HELP                     = 0;
const uint8 C_INT_USE_COMPRESSION               = 1;
const uint8 C_INT_STORE_EXTERNALS_TOPOLOGY      = 2;
const uint8 C_INT_SPECIFY_INTERFACE             = 3;
const uint8 C_INT_SPECIFY_CONFIG_PATH           = 4;
const uint8 C_INT_SPECIFY_NS_RECIPIENT          = 5;
const uint8 C_INT_CALCULATE_TCP_RTT             = 6;
const uint8 C_INT_REPLAY_MODE                   = 7;
}
#endif
