/*
 * DisServiceStats.h
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

#ifndef INCL_DIS_SERVICE_STATS_H
#define INCL_DIS_SERVICE_STATS_H

#include "Listener.h"

#include "DisServiceStatus.h"

#include "DArray2.h"
#include "LoggingMutex.h"
#include "StringHashtable.h"

namespace IHMC_ACI
{
    class DisServiceMsg;

    class DisServiceStats : public MessageListener
    {
        public:
            virtual ~DisServiceStats (void);

            void newIncomingMessage (const void *, uint16, DisServiceMsg *pDisServiceMsg,
                                     uint32, const char *);

            struct Stats
            {
                Stats (void);
                uint32 ui32ClientMessagesPushed;
                uint32 ui32ClientBytesPushed;
                uint32 ui32ClientMessagesMadeAvailable;
                uint32 ui32ClientBytesMadeAvailable;
                uint32 ui32FragmentsPushed;
                uint32 ui32FragmentBytesPushed;
                uint32 ui32OnDemandFragmentsSent;
                uint32 ui32OnDemandFragmentBytesSent;
                uint32 ui32TargetedDuplicateTraffic;
                uint32 ui32OverheardDuplicateTraffic;
            };

        private:
            friend class DisseminationService;
            friend class DataRequestServer;
            friend class DisServiceStatusNotifier;
            friend class MessageReassembler;
            friend class TransmissionServiceListener;

            DisServiceStats (void);
            void lock (void);
            void unlock (void);
            void messagePushedByClient (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint32 ui32Size);
            void messageMadeAvailableByClient (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint32 ui32Size);
            void fragmentPushed (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint16 ui16Size);
            void onDemandFragmentPushed (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint16 ui16Size);
            void dataFragmentReceived (const char *pszRemoteNodeId, const char *pszGroupName, uint16 ui16Tag, uint16 ui16Size);
            void dataMessageReceived (const char *pszRemoteNodeId, const char *pszGroupName, uint16 ui16Tag, uint32 ui32Size);
            void acknowledgmentMessageSent (uint16 ui16Size);
            void acknowledgmentMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size);
            void ctrlToCtrlMessageSent (uint16 ui16Size);
            void ctrlToCtrlMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size);
            void missingFragmentRequestSent (uint16 ui16Size);
            void missingFragmentRequestReceived (const char *pszRemoteNodeId, uint16 ui16Size);
            void dataCacheQueryMessageSent (const char *pszTargetNodeId, uint16 ui16Size);
            void dataCacheQueryMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size);
            void dataCacheQueryMessageReplySent (uint16 ui16Size);
            void dataCacheQueryMessageReplyReceived (const char *pszRemoteNodeId, uint16 ui16Size);
            void topologyStateMessageSent (uint16 ui16Size);
            void topologyStateMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size);
            void keepAliveMessageSent (void);
            void keepAliveMessageReceived (const char *pszRemoteNodeId);
            void queryMessageSent (uint16 ui16Size);
            void queryMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size);
            void queryHitsMessageSent (uint16 ui16Size);
            void queryHitsMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size);

            void dataMessageReceived (const char *pszRemoteNodeId);
            void dataMessageForwarded (void);

        private:
            // Methods internal to DisServiceStats
            Stats * getStatsForClientGroupTag (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag);

        private:
            typedef NOMADSUtil::DArray2<Stats> TagStats;                         // Statistics on a per-tag basis
            typedef NOMADSUtil::StringHashtable<TagStats> GroupTagStats;         // Statistics on a per-group and per-tag basis
            typedef NOMADSUtil::DArray2<GroupTagStats> ClientGroupTagStats;      // Statistics on a per-client, per-group, and per-tag basis

            NOMADSUtil::LoggingMutex _m;
            Stats _overallStats;
            ClientGroupTagStats _clientGroupTagStats;
            uint32 _ui32DataMessagesReceived;
            uint32 _ui32DataBytesReceived;
            uint32 _ui32DataFragmentsReceived;
            uint32 _ui32DataFragmentBytesReceived;
            uint32 _ui32AcknowledgmentMessageSent;
            uint32 _ui32AcknowledgmentMessageBytesSent;
            uint32 _ui32AcknowledgmentMessageReceived;
            uint32 _ui32AcknowledgmentMessageBytesReceived;
            uint32 _ui32CtrlToCtrlMessageSent;
            uint32 _ui32CtrlToCtrlMessageBytesSent;
            uint32 _ui32CtrlToCtrlMessageReceived;
            uint32 _ui32CtrlToCtrlMessageBytesReceived;
            uint32 _ui32MissingFragmentRequestMessagesSent;
            uint32 _ui32MissingFragmentRequestBytesSent;
            uint32 _ui32MissingFragmentRequestMessagesReceived;
            uint32 _ui32MissingFragmentRequestBytesReceived;
            uint32 _ui32DataCacheQueryMessagesSent;
            uint32 _ui32DataCacheQueryBytesSent;
            uint32 _ui32DataCacheQueryMessagesReceived;
            uint32 _ui32DataCacheQueryBytesReceived;
            uint32 _ui32DataCacheQueryMessagesReplySent;
            uint32 _ui32DataCacheQueryReplyBytesSent;
            uint32 _ui32DataCacheQueryMessagesReplyReceived;
            uint32 _ui32DataCacheQueryReplyBytesReceived;
            uint32 _ui32TopologyStateMessagesSent;
            uint32 _ui32TopologyStateBytesSent;
            uint32 _ui32TopologyStateMessagesReceived;
            uint32 _ui32TopologyStateBytesReceived;
            uint32 _ui32KeepAliveMessagesSent;
            uint32 _ui32KeepAliveMessagesReceived;
            uint32 _ui32QueryMessageSent;
            uint32 _ui32QueryMessageBytesSent;
            uint32 _ui32QueryMessageReceived;
            uint32 _ui32QueryMessageBytesReceived;
            uint32 _ui32QueryHitsMessageSent;
            uint32 _ui32QueryHitsMessageBytesSent;
            uint32 _ui32QueryHitsMessageReceived;
            uint32 _ui32QueryHitsMessageBytesReceived;

            // Variables to monitor the forward probability
            uint32 _ui32TotDataFragRcvd;
            uint32 _ui32DataFragFrwded;

            uint32 _ui32TargetedDuplicateTraffic;
            uint32 _ui32OverheardDuplicateTraffic;

            NOMADSUtil::StringHashtable<DisServiceBasicStatisticsInfoByPeer> _statsByPeer;
    };
}

#endif   // #ifndef INCL_DIS_SERVICE_STATS_H
