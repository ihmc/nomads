#ifndef NETSENSOR_TCPStream__INCLUDED
#define NETSENSOR_TCPStream__INCLUDED
/*
* TCPRttContainer.h
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
*/

#include "StrClass.h"
#include "NetSensorPacket.h"
#include "NetworkHeaders.h"
#include "TCPRTTList.h"
#include "NetSensorUtilities.h"
#include "InetAddr.h"
#include "NLFLib.h"
#include "FINHandshake.h"
#include "TCPFlagUtil.h"

namespace IHMC_NETSENSOR
{
    struct TCPStreamData
    {
        NOMADSUtil::String sLocalIP;
        NOMADSUtil::String sRemoteIP;
        NOMADSUtil::String sLocalPort;
        NOMADSUtil::String sRemotePort;

        bool operator==(const TCPStreamData & rhs)
        {
            return rhs.sLocalIP == sLocalIP && rhs.sRemoteIP == sRemoteIP &&
                rhs.sLocalPort == sLocalPort && rhs.sRemotePort == sRemotePort;
        }
    };

    class TCPStream
    {
    public:
        TCPStream(TCPStreamData & streamData, uint32 ui32Resolution);
        uint32 clean(uint32 ui32MaxCleaningNumber);
        uint8 getCount(void);
        TCPStreamData getData(void);
        TCPRTTList* getRTTTable(void);
        bool hasClosed(void);
        bool operator== (const TCPStream & rhs);
        void print(void);
        void updateStreamRTTs (NOMADSUtil::IPHeader & ipHeader, int64 rcvTime, bool isIncoming);
        NOMADSUtil::PtrLList<measure::Measure>* createMeasures(NOMADSUtil::String sSensorIP);

    private:
        void addToRTTTable(uint32 ui32SeqNum, uint16 ui16TCPDataLength, int64 i64Timestamp);
        void checkRTTTable(uint32 ui32AckNum, int64 i64Timestamp);

        //<-------------------------------------------------->
        bool _bIsClosed;
        TCPStreamData _data;
        TCPRTTList _rttTable;
        FINHandshake _finHandshake;
    public:
        int64 i64LastUpdateTime;
    };

    inline bool TCPStream::operator==(const TCPStream & rhs)
    {
        return _data == rhs._data;
    }

    inline TCPRTTList* TCPStream::getRTTTable()
    {
        return &_rttTable;
    }
}
#endif

