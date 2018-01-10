#ifndef NETSENSOR_ICMPHashTable__INCLUDED
#define NETSENSOR_ICMPHashTable__INCLUDED
/*
* ICMPHashTable.h
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
* This class implements a four level hash map that will contain
* stats identified by source ip, dest ip, type, and code
*/

#include"StringHashtable.h"
#include"PacketStructures.h"
#include"ICMPTypesContainer.h"
#include"icmpinfo.pb.h"

namespace IHMC_NETSENSOR
{
    typedef  NOMADSUtil::StringHashtable<ICMPTypesContainer> codeLevel;
    typedef  NOMADSUtil::StringHashtable<codeLevel> typeLevel;
    typedef  NOMADSUtil::StringHashtable<typeLevel> destAddrLevel;
    typedef  NOMADSUtil::StringHashtable<destAddrLevel> srcAddrLevel;

    class ICMPHashTable
    {
    public:
        ICMPHashTable(void);
        ~ICMPHashTable(void);

        void   fillICMPProto(netsensor::ICMPPacketsByInterface *pIpbi);
        void   fillProtoHead(IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket *pPacket, netsensor::ProtoIpHeader *pIPHead);
        void   fillProtoDatagram(IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket *pPacket, netsensor::ProtoDatagramInfo *pDI);
        void   print(void);
        void   put(ICMPTypesContainer *pICMPTypeContainer);
        uint8  getCount(void); // Debugging purposes
        uint32 cleanTable(const uint32 ui32CleaningNumber);
        ICMPTypesContainer* get(IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket *pIcmpPacket);
    private:
        srcAddrLevel _icmpHashTable;
        NOMADSUtil::Mutex _pTMutex;
    };
}
#endif
