#ifndef NETSENSOR_ICMPTable__INCLUDED
#define NETSENSOR_ICMPTable__INCLUDED
/*
* ICMPInterfaceTable.h
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
#include "ICMPRTTHashTable.h"
#include "measure.pb.h"

#include"ICMPTypesContainer.h"

namespace IHMC_NETSENSOR
{
    class ICMPInterfaceTable
    {
    public:
        ICMPInterfaceTable(const uint32 timeInterval);

        void fillICMPProtoObject(const char* pIName, netsensor::ICMPPacketsByInterface *pIpbi);
        ICMPRTTHashTable* getRTTTableForInterface(const char *pInterfaceName);

        void cleanTable(const uint32 ui32CleaningNumber);
        void put(const char *pInterfaceName, IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket *pIcmpPacket, bool isSent, int64 timestamp);
        void printContent(void);
        void printRTTContent(void);
        bool mutexTest(void);
    private:
        void addToRTTTable(const char *pInterfaceName, IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket *pIcmpPacket, bool isSent, int64 timestamp);
        void addToTypeTable(const char *pInterfaceName, IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket *pIcmpPacket);

        //<--------------------------------------------------------------------------------------------->

        NOMADSUtil::Mutex _pTMutex;
        NOMADSUtil::StringHashtable<ICMPHashTable> _icmpInfoTableContainer;
        NOMADSUtil::StringHashtable<ICMPRTTHashTable> _icmpRTTTable;
        uint32 _ui32msResolution;
    };
}

#endif