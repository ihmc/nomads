#ifndef NETSENSOR_TCPRTTTable__INCLUDED
#define NETSENSOR_TCPRTTTable__INCLUDED
/*
* TCPRttTable.h
* Author: bordway@ihmc.us rfronteddu@ihmc.us
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
* The TCPRTTTable is a tiered hashtable containing the source ip, dest ip,
* source port, dest port, and finally the object for the TCP calculations.
* This class stores outgoing TCP information, and calculates the RTT when
* an incoming TCP packet is received. It does not work in the opposite way.
*
*/
#include "StringHashtable.h"
#include "LList.h"
#include "PacketStructures.h"
#include "StrClass.h"
#include "Mutex.h"
#include "NetSensorConstants.h"
#include "TCPRTTContainer.h"

namespace IHMC_NETSENSOR
{
    typedef NOMADSUtil::StringHashtable<TCPRTTContainer>    DPLevel;
    typedef NOMADSUtil::StringHashtable<DPLevel>  SPLevel;
    typedef NOMADSUtil::StringHashtable<SPLevel>  DIPLevel;
    typedef NOMADSUtil::StringHashtable<DIPLevel> SIPLevel;

    struct TCPRTTData
    {
        NOMADSUtil::String sSourceIP;
        NOMADSUtil::String sDestIP;
        NOMADSUtil::String sSourcePort;
        NOMADSUtil::String sDestPort;

        int64 i64rcvTime;
    };


    class TCPRTTTable
    {
    public:
        TCPRTTTable(void);
        ~TCPRTTTable(void);

        uint32 cleanTables(const uint32 ui32MaxCleaningNumber); 
        uint8 getCount(void);
        void   put(TCPRTTData & pTCPStruct, bool isSent);
        void   print(void);
    private:
        void checkSent(TCPRTTData & pTCPStruct);
        void checkReceived(TCPRTTData & pTCPStruct);

        //<---------------------------------------------->
        SIPLevel _tcpRTTTable;
        NOMADSUtil::Mutex _mutex;
    };
}

#endif

