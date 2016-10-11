/*
 * DisServiceStatus.java
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

package us.ihmc.aci.disService.monitor;

/**
 *
 * @author nsuri
 */
public class DisServiceStatus
{
    public static final int DIS_SERVICE_STATUS_HEADER_BYTE = 0x00D1;   // This will be the first byte of every DisService status packet
    public static final int DEFAULT_DIS_SERVICE_STATUS_PORT = 1401;

    public static enum DisServiceStatusNoticeType
    {
        DSSNT_Undefined,
        DSSNT_ClientConnected,
        DSSNT_ClientDisconnected,
        DSSNT_SummaryStats,
        DSSNT_DetailedStats,
        DSSNT_TopologyStatus
    }

    public static enum DisServiceStatusFlags
    {
        DSSF_Undefined,
        DSSF_End,
        DSSF_OverallStats,
        DSSF_PerClientGroupTagStats,
        DSSF_DuplicateTrafficInfo
    }

    public static class DisServiceBasicStatisticsInfo
    {
        DisServiceBasicStatisticsInfo (String peerId)
        {
            this.peerId = peerId;
        }

        public String peerId;
        public long dataMessagesReceived;
        public long dataBytesReceived;
        public long dataFragmentsReceived;
        public long dataFragmentBytesReceived;
        public long missingFragmentRequestMessagesSent;
        public long missingFragmentRequestBytesSent;
        public long missingFragmentRequestMessagesReceived;
        public long missingFragmentRequestBytesReceived;
        public long dataCacheQueryMessagesSent;
        public long dataCacheQueryBytesSent;
        public long dataCacheQueryMessagesReceived;
        public long dataCacheQueryBytesReceived;
        public long topologyStateMessagesSent;
        public long topologyStateBytesSent;
        public long topologyStateMessagesReceived;
        public long topologyStateBytesReceived;
        public long keepAliveMessagesSent;
        public long keepAliveMessagesReceived;
        public long queryMessagesSent;
        public long queryMessagesReceived;
        public long queryHitsMessagesSent;
        public long queryHitsMessagesReceived;
    }

    // The following is utilized for both overall stats as well as stats per client, group, and tag
    public static class DisServiceStatsInfo
    {
        DisServiceStatsInfo (String peerId)
        {
            this.peerId = peerId;
        }

        public String peerId;
        public long clientMessagesPushed;
        public long clientBytesPushed;
        public long clientMessagesMadeAvailable;
        public long clientBytesMadeAvailable;
        public long fragmentsPushed;
        public long fragmentBytesPushed;
        public long onDemandFragmentsSent;
        public long onDemandFragmentBytesSent;
    }

    public static class DisServiceDuplicateTrafficInfo
    {
        DisServiceDuplicateTrafficInfo (String peerId)
        {
            this.peerId = peerId;
        }

        public String peerId;
        public long targetedDuplicateTraffic;
        public long overheardDuplicateTraffic;
    }

    public static class DisServiceBasicStatisticsInfoByPeer
    {
        DisServiceBasicStatisticsInfoByPeer (String peerId)
        {
            this.peerId = peerId;
        }

        public String peerId;
        public String remotePeerId;
        public long dataMessagesReceived;
        public long dataBytesReceived;
        public long dataFragmentsReceived;
        public long dataFragmentBytesReceived;
        public long missingFragmentRequestMessagesReceived;
        public long missingFragmentRequestBytesReceived;
        public long keepAliveMessagesReceived;
    }

    public static class DisServiceClientGroupTagStatsInfoHeader
    {
        DisServiceClientGroupTagStatsInfoHeader (String peerId)
        {
            this.peerId = peerId;
        }

        public String peerId;
        public int clientId;
        public int tag;
        public int groupNameLength;        // Bytes for the group name will follow the struct
    }
}
