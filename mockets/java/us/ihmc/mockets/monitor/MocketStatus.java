/*
 * MocketStatus.java
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
 *
 * @author ebenvegnu
 */


package us.ihmc.mockets.monitor;

import java.net.InetAddress;

public class MocketStatus
{
    public static final int DEFAULT_MOCKET_STATUS_PORT = 1400;

    public static enum MocketStatusNoticeType
    {
        MSNT_Undefined,
        MSNT_ConnectionFailed,
        MSNT_ConnectionEstablished,
        MSNT_ConnectionReceived,
        MSNT_Stats,
        MSNT_Disconnected,
        MSNT_ConnectionRestored
    };
    
    public static enum MocketStatusFlags
    {
        MSF_Undefined,
        MSF_End,
        MSF_OverallMessageStatistics,
        MSF_PerTypeMessageStatistics
    };
    
    public static class EndPointsInfo
    {
        public long PID;
        public String identifier;
        public long localAddr;
        public long localPort;
        public long remoteAddr;
        public long remotePort;
        public InetAddress localAddress;
        public InetAddress remoteAddress;
    };

    public static class MocketStatisticsInfo
    {
        public long lastContactTime;
        public long sentBytes;
        public long sentPackets;
        public long retransmits;
        public long receivedBytes;
        public long receivedPackets;
        public long duplicatedDiscardedPackets;
        public long noRoomDiscardedPackets;
        public long reassemblySkippedDiscardedPackets;
        public float estimatedRTT;
        public long unacknowledgedDataSize;
        public long unacknowledgedQueueSize; // Update missing
        public long pendingDataSize;
        public long pendingPacketQueueSize;
        public long reliableSequencedDataSize;
        public long reliableSequencedPacketQueueSize;
        public long reliableUnsequencedDataSize;
        public long reliableUnsequencedPacketQueueSize;
    };

    public static class MessageStatisticsInfo
    {
        public int msgType;
        public long sentReliableSequencedMsgs;
        public long sentReliableUnsequencedMsgs;
        public long sentUnreliableSequencedMsgs;
        public long sentUnreliableUnsequencedMsgs;
        public long receivedReliableSequencedMsgs;
        public long receivedReliableUnsequencedMsgs;
        public long receivedUnreliableSequencedMsgs;
        public long receivedUnreliableUnsequencedMsgs;
        public long cancelledPackets;
    };   
    
    public static class Update
    {
        public EndPointsInfo epi;
        public MocketStatisticsInfo msi;
    }
}
