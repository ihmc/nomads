#ifndef NETSENSOR_ICMPRTTInterfaceTable__INCLUDED
#define NETSENSOR_ICMPRTTInterfaceTable__INCLUDED
/*
* ICMPRTTInterfaceTable.h
* Author: bordway@ihmc.us
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
* ICMP table that will control ICMP information
*/

#include"StrClass.h"
#include"Mutex.h"
#include"StringHashtable.h"
#include"NetworkHeaders.h"
#include"PacketStructures.h"
#include"ICMPHashTable.h"
#include"icmpinfo.pb.h"
#include "ICMPRTTTable.h"

#include"ICMPTypesContainer.h"

namespace IHMC_NETSENSOR
{
    class ICMPRTTInterfaceTable
    {
    public:
        ICMPRTTInterfaceTable(const uint32 timeInterval);

        void fillICMPProtoObject(const char* pIName, netsensor::ICMPPacketsByInterface *pIpbi);
        void cleanTable(const uint32 ui32CleaningNumber);
        void put(const char *pInterfaceName, IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket *pIcmpPacket, bool isIncoming);
        void printContent(void);
        bool mutexTest(void);
    private:
        NOMADSUtil::Mutex _pTMutex;
        NOMADSUtil::StringHashtable<ICMPRTTTable> _rttTable;

        uint32 _ui32msResolution;
    };
}

#endif