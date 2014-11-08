#ifndef INCL_MOCKET_STATUS_H
#define INCL_MOCKET_STATUS_H

/*
 * MocketStatus.h
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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

#include "FTypes.h"
#include "StrClass.h"


enum MocketStatusNoticeType
{
    MSNT_Undefined = 0x00,
    MSNT_ConnectionFailed = 0x01,
    MSNT_ConnectionEstablished = 0x02,
    MSNT_ConnectionReceived = 0x03,
    MSNT_Stats = 0x04,
    MSNT_Disconnected = 0x05,
    MSNT_ConnectionRestored = 0x06
};

enum MocketStatusFlags
{
    MSF_Undefined = 0x00,
    MSF_End = 0x01,
    MSF_OverallMessageStatistics = 0x02,
    MSF_PerTypeMessageStatistics = 0x03
};

#pragma pack (push,1)
struct EndPointsInfo
{
    uint32 ui32LocalAddr;
    uint16 ui16LocalPort;
    uint32 ui32RemoteAddr;
    uint16 ui16RemotePort;
};

struct StatisticsInfo
{
    int64  i64LastContactTime;
    uint32 ui32SentBytes;
    uint32 ui32SentPackets;
    uint32 ui32Retransmits;
    uint32 ui32ReceivedBytes;
    uint32 ui32ReceivedPackets;
    uint32 ui32DuplicatedDiscardedPackets;
    uint32 ui32NoRoomDiscardedPackets;
    uint32 ui32ReassemblySkippedDiscardedPackets;
    float  fEstimatedRTT;

    // The following will only available with Stream Mockets (in the future!)
    uint32 ui32UnacknowledgedDataSize;

    // The following are only available with MessageMockets
    uint32 ui32PendingDataSize;
    uint32 ui32PendingPacketQueueSize;
    uint32 ui32ReliableSequencedDataSize;
    uint32 ui32ReliableSequencedPacketQueueSize;
    uint32 ui32ReliableUnsequencedDataSize;
    uint32 ui32ReliableUnsequencedPacketQueueSize;
};

struct MessageStatisticsInfo
{
    uint16 ui16MsgType;
    uint32 ui32SentReliableSequencedMsgs;
    uint32 ui32SentReliableUnsequencedMsgs;
    uint32 ui32SentUnreliableSequencedMsgs;
    uint32 ui32SentUnreliableUnsequencedMsgs;
    uint32 ui32ReceivedReliableSequencedMsgs;
    uint32 ui32ReceivedReliableUnsequencedMsgs;
    uint32 ui32ReceivedUnreliableSequencedMsgs;
    uint32 ui32ReceivedUnreliableUnsequencedMsgs;
    uint32 ui32CancelledPackets;
};

#pragma pack (pop)

#endif   // #ifndef INCL_MOCKET_STATUS_H
