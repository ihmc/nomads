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
#include "UInt32Hashtable.h"
#include "StringHashtable.h"
#include "ProtobufWrapper.h"
#include "LList.h"
#include "PtrLList.h"
#include "PacketStructures.h"
#include "StrClass.h"
#include "Mutex.h"
#include "NetSensorConstants.h"
#include "RTTContainer.h"
#include "UInt32Hashtable.h"
#include "measure.pb.h"

namespace IHMC_NETSENSOR
{
    typedef NOMADSUtil::UInt32Hashtable<RTTContainer> ACKLevel;
    typedef NOMADSUtil::UInt32Hashtable<ACKLevel>  SeqLevel;

    class TCPRTTTable
    {
    public:
        TCPRTTTable(uint32 ui32msResolution);

        uint32 cleanTables (const uint32 ui32MaxCleaningNumber); 
        uint8 getCount(void);
        bool hasAckNum (uint32 ui32AckNum);
        float getAvgRTT(void);
        uint32 getMinRTT(void);
        uint32 getMaxRTT(void);
        uint32 getMostRecentRTT(void);
        void print(void);
        void putNewAckEntry (uint32 ui32AckNum, const int64 i64RcvTimestamp);
        void putNewSeqEntry (uint32 ui32SeqNum, uint32 ui32NextAckNum,
            const int64 i64SntTimestamp);


    private:
        //<---------------------------------------------->
        SeqLevel _tcpRTTTable;
        TimeIntervalAverage<uint32> _avgRttTIA;
        NOMADSUtil::Mutex _mutex;
    };
}

#endif

