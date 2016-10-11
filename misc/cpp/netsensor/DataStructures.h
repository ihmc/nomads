/*
 * DataStructures.cpp
 *
 * This file is part of the IHMC NetSensor Library/Component
 * Copyright (c) 2010-2016 IHMC.
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
 */




#ifndef INCL_NET_SENSOR_DATASTRUCTURE_H
#define INCL_NET_SENSOR_DATASTRUCTURE_H

#include "TimeIntervalAverage.h"
#include "UInt32Hashtable.h"
#include "StringHashtable.h"
#include "LList.h"
namespace IHMC_MISC
{
    //Table structures
    struct PerSequenceNumber
    {
        PerSequenceNumber(void);
        int64 i64TRequestTime;
        int64 i64TReplyTime;
        int64 i64TimeOfLastChange;
        bool gotAReply;
        bool gotARequest;
    };
    struct PerID
    {
        PerID(void);
        NOMADSUtil::UInt32Hashtable<PerSequenceNumber> _PerSequenceNumber;
        int64 averageFlightTimePerID;
        uint16 percentageOfPacketLostPerID;
    };
    struct DestField
    {
        DestField(void);
		int64 i64LastMeasuredRTT;
        NOMADSUtil::UInt32Hashtable<PerID> _perID;
    };
    struct RoundTripStat
    {
        RoundTripStat(void);
        NOMADSUtil::UInt32Hashtable<DestField> _PerDest;
    };
    struct PerNodeTrafficStats
    {
        PerNodeTrafficStats(void);
        TimeIntervalAverage<uint32> tiaFiveSecs;
		TimeIntervalAverage<uint32> tiaOneMinute;
		TimeIntervalAverage<uint32> tiaNumberOfPacketsInFiveSec;
        int64 i64TimeOfLastChange;
    };
    struct PerNodeTrafficStatsbyProtocol
    {
        PerNodeTrafficStatsbyProtocol(void);
        NOMADSUtil::UInt32Hashtable<PerNodeTrafficStats> _PerNodeTrafficStatsbyPort;
    };
    struct PerNodeTrafficStatsContainer
    {
        PerNodeTrafficStatsContainer(void);
        NOMADSUtil::UInt32Hashtable<PerNodeTrafficStatsbyProtocol> _PerNodeTrafficStatsbyProtocol;
    };

    struct PerNodeTrafficStatsIPA
    {
        PerNodeTrafficStatsIPA(void);
        NOMADSUtil::UInt32Hashtable<PerNodeTrafficStats> _PerNodeTrafficStatsbyIPB;
    };

    struct PerNodeTrafficStatsIP2
    {
        PerNodeTrafficStatsIP2(void);
        NOMADSUtil::UInt32Hashtable<PerNodeTrafficStatsbyProtocol> _byProtocol;
    };
    struct PerNodeTrafficStatsIP1
    {
        PerNodeTrafficStatsIP1(void);
        NOMADSUtil::UInt32Hashtable<PerNodeTrafficStatsIP2> _IPB;
    };

    //FOR MAC ARP
    struct IpEntry
    {
        IpEntry(void);
        uint32 ui32Addr;
    };


    struct macPerIp
    {
        macPerIp(void);
        NOMADSUtil::LList<uint32> ips;
    };

    struct IpPerGW
    {
        IpPerGW(void);
        NOMADSUtil::String _IpEntry;
    };




    //FOR MAC TRAFFIC

    struct DestMac
    {
        DestMac(void);
        TimeIntervalAverage<uint32> tiaFiveSecs;
        TimeIntervalAverage<uint32> tiaOneMinute;
        int64 i64TimeOfLastChange;
    };

    struct SourceMac
    {
        SourceMac(void);
        NOMADSUtil::StringHashtable<DestMac> _destMac;
    };


    struct Mac
    {
        Mac(void);
        NOMADSUtil::String mac;
        int64 i64TimeOfLastChange;
    };
}

#endif