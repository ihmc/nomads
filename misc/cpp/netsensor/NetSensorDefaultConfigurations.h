/*
* NetSensorDefaultConfigurations.h
* Author: rfronteddu@ihmc.us
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
* This class contains all the netsensor default configurations
*/
#ifndef NETSENSOR_NetSensorDefaultConfigurations__INCLUDED
#define NETSENSOR_NetSensorDefaultConfigurations__INCLUDED
#include "FTypes.h"
namespace IHMC_NETSENSOR
{
    //static  const char* DEFAULT_MONITORED_INTERFACE        = "Local Area Connection";
    static const char *   DEFAULT_MONITORED_INTERFACE = "Ethernet";
    static const char *   DEFAULT_DELIVERY_IP = "127.0.0.1";
    static const uint32   DEFAULT_VERSION = 2;
    static const uint32   DEFAULT_MTU = 1400;
    static const uint32   DEFAULT_TRAFFIC_DELIVERY_PERIOD = 1000;
    static const uint32   DEFAULT_TOPOLOGY_DELIVERY_PERIOD = 1000;
    static const uint32   DEFAULT_DELIVERY_PORT = 7777;
    static const bool     DEFAULT_TOPOLOGY_LAX_ACTIVE = false;
    static const bool     DEFAULT_TOPOLOGY_ARP_ACTIVE = false;
    static const bool     DEFAULT_TOPOLOGY_NETMASK_ACTIVE = true;
    static const bool     DEFAULT_TOPOLOGY_NETPROXY_ACTIVE = false;
    static const bool     DEFAULT_RTT_ICMP_ACTIVE = false;
    static const bool     DEFAULT_TRAFFIC_MULTICAST_IGNORE = false;
    static const bool     DEFAULT_EXTERNAL_TOPOLOGY_IGNORE = false;
    static const bool     DEFAULT_OUTPUT_COMPRESSION_ACTIVE = false;
    static const bool     DEFAULT_TCP_RTT_CALCULATION_ACTIVE = false;
    static const uint32   DEFAULT_NOT_ACTIVE = 0;
}
#endif