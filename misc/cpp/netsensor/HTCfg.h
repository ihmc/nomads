#ifndef NETSENSOR_HandlerThreadConf__INCLUDED
#define NETSENSOR_HandlerThreadConf__INCLUDED
/*
* HTCfg.h
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
*
*
* The handler thread retrive packets from the packet queue
* and proceeds in populating the stats tables
*
*/
#include "ICMPInterfaceTable.h"
#include "NetSensorPacketQueue.h"
#include "TCPRTTInterfaceTable.h"
#include "TopologyTable.h"
#include "TopologyCache.h"
#include "TrafficTable.h"

namespace IHMC_NETSENSOR
{
class HTCfg
{
public:
    NetSensorPacketQueue * pQueue;
    NetSensorPacketQueue * pRttQueue;
    TrafficTable         * pTrT;
    TopologyTable        * pTopT;
    ICMPInterfaceTable   * pIcmpTable;
    TopologyCache        * pTopCache;
    TCPRTTInterfaceTable * pTRIT;
    bool storeExternals;
    bool calculateTCPRTT;
};
}
#endif
