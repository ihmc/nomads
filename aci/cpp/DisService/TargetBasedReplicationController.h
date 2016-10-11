/**
 * TargetBasedReplicationController.h
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
 *
 * Configuration parameters
 *
 *  aci.disService.replicationTarget.includeList     default=emptyList
 *  aci.disService.replicationTarget.excludeList     default=emptyList
 *  aci.disService.replication.checkTargetForMsgs    default=true
 *  aci.disService.replication.requireAcks           default=true
 *  aci.disService.replication.concurrentReplication default=false (i.e. serial)
 *  aci.disService.replication.concurrentReception   default=false (i.e. serial)
 *
 * Author: Marco Marchini    (mmarchini@ihmc.us)
 * Created on April 18, 2012
 */

#ifndef INCL_TARGET_BASED_REPLICATION_CONTROLLER_H
#define INCL_TARGET_BASED_REPLICATION_CONTROLLER_H

#include "DataCacheReplicationController.h"
#include "ReceivedMessages.h"

#include "ManageableThread.h"
#include "Mutex.h"
#include "PtrLList.h"
#include "PtrQueue.h"
#include "StrClass.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class DisseminationService;
    class ReplicationStartReplyMsg;
    class TransmissionHistoryInterface;

    class TargetBasedReplicationController : public DataCacheReplicationController
    {
        public:
            static const uint8 CONTROLLER_TYPE = DCRC_TargetBased;
            static const uint8 CONTROLLER_VERSION = 0x01;
            static const uint32 DEFAULT_ACK_TIMEOUT = 60000;    // One minute
            static const uint8 DEFAULT_NUM_OF_MSGS_TO_ACK = 10; // The number of messages to ack in one batch

            TargetBasedReplicationController (DisseminationService *pDisService, NOMADSUtil::ConfigManager *pCfgMgr);
            ~TargetBasedReplicationController (void);

            void newNeighbor (const char *pszNodeUUID, const char *pszPeerRemoteAddr,
                              const char *pszIncomingInterface);
            void deadNeighbor (const char *pszNodeUUID);
            void newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                    const char *pszIncomingInterface);
            void droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr);
            void stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate);
            void dataCacheUpdated (MessageHeader *pMH, const void *pPayload);

        protected:
            void disServiceControllerMsgArrived (ControllerToControllerMsg *pCtrlMsg);
            void disServiceControlMsgArrived (DisServiceCtrlMsg *pCtrlMsg);
            void disServiceDataMsgArrived (DisServiceDataMsg *pDataMsg);

        private:
            /**
             * Loads parameters from the config file.
             * Returns 0 if the operation was successful, a negative number otherwise.
             */
            int configure (NOMADSUtil::ConfigManager *pCfgMgr);

            /**
             * Checks if the node id passed as parameter is selected by the target specification lists.
             * Returns true if replication should occur, false otherwise.
             */
            bool evaluatePeerForReplication (const char *pszNodeUUID);

            int checkAndStartNextReplicationSession (void);
            int checkAndStartNextReceivingSession (void);

            bool cleanupTerminatedReplicators (void);

        protected:
            class ReplicationSessionThread : public NOMADSUtil::ManageableThread
            {
                public:
                    ReplicationSessionThread (TargetBasedReplicationController *pTBRepCtlr);
                    ~ReplicationSessionThread (void);

                    int init (const char *pszLocalNodeID, const char *pszTargetNodeID, bool bCheckTargetForMsgs, bool bRequireAcks);

                    void run (void);

                    const char * getTargetNodeID (void);

                    int targetDied (void);

                    int replyMsgArrived (ReplicationStartReplyMsg *pReplyMsg);

                    bool operator == (const ReplicationSessionThread &rhsRST);

                private:
                    TargetBasedReplicationController *_pTBRepCtlr;
                    NOMADSUtil::String _localNodeID;   // My nodeID
                    NOMADSUtil::String _targetNodeID;  // Peer nodeID receiving replication data
                    bool _bCheckTargetForMsgs;  // If true, sender expects peer to send list of messages
                                                //          that do not need to be sent
                    bool _bRequireAcks;  // If true, sender expects peer to acknowledge receipt of messages

                    NOMADSUtil::Mutex _m;
                    NOMADSUtil::ConditionVariable _cv;
                    bool _bReplicating;  // Set to true when ReplicationStartReplyMsg is received
                    bool _bTargetDead;   // If true, peer node has been detected as a deadNeighbor

                    NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *_pMsgsToExclude;
            };

            class ReceiverSession
            {
                public:
                    ReceiverSession (TargetBasedReplicationController *pTBRepCtlr);
                    ~ReceiverSession (void);

                    int init (const char *pszLocalNodeID, const char *pszSourceNodeID, bool bSendCurrentDataList, bool bGenerateAcks);

                    int startReceiving (void);

                    void dataCacheUpdated (MessageHeader *pMH, const void *pPayload);

                    void sessionEnding (void);

                    bool operator == (const ReceiverSession &rhsRS);
                private:
                    void sendAckMsg (void);

                private:
                    TargetBasedReplicationController *_pTBRepCtlr;
                    NOMADSUtil::String _localNodeID;
                    NOMADSUtil::String _sourceNodeID;
                    bool _bSendCurrentDataList;    // replicator has requested reply to include message list
                    bool _bGenerateAcks;           // replicator has requested data message receipt acknowledgements
                    NOMADSUtil::Mutex _mMsgsToAck;
                    NOMADSUtil::PtrQueue<NOMADSUtil::String> _msgsToAck;
                    int64 _i64LastAckTime;
            };

            friend class ReplicationSessionThread;
            void addTerminatingReplicator (ReplicationSessionThread *pRST);

        private:
            DisseminationService *_pDisService;
            TransmissionHistoryInterface *_pTransmissionHistory; 
            ReceivedMessagesInterface *_pReceivedMessages;
            bool _bInitialized;

            NOMADSUtil::PtrLList<NOMADSUtil::String> _targetIncludeSpec;  // Specification of the target nodes that we should replicate to
            NOMADSUtil::PtrLList<NOMADSUtil::String> _targetExcludeSpec;  // Specification of the target nodes that we should NOT replicate to

            bool _bCheckTargetForMsgs;              // Check the contents of the target before replicating
            bool _bRequireAcks;                     // Require acknowledgement for replicated messages
            bool _bAllowConcurrentReplication;      // if false, operate replicator sessions serially
            bool _bAllowConcurrentReception;        // if false, operate receiver sessions serially
            NOMADSUtil::Mutex _mReplicators;
            NOMADSUtil::StringHashtable<ReplicationSessionThread> _replicators;     // When operating serially, there will only be one element here
            NOMADSUtil::Mutex _mTerminatingReplicators;
            NOMADSUtil::PtrLList<ReplicationSessionThread> _terminatingReplicators;
            NOMADSUtil::Mutex _mReceivers;
            NOMADSUtil::StringHashtable<ReceiverSession> _receivers;                // When operating serially, there will only be one element here
            NOMADSUtil::Mutex _mPendingDestNodes;
            NOMADSUtil::PtrQueue<NOMADSUtil::String> _pendingDestNodes;             // Destination nodes that are awaiting replication from this node (when operating serially)
            NOMADSUtil::Mutex _mPendingSourceNodes;
            NOMADSUtil::StringHashtable<ReceiverSession> _pendingSourceNodes;       // Source nodes that are pending replicating to this node (when operating serially)
    };

}

#endif   // #ifndef INCL_TARGET_BASED_REPLICATION_CONTROLLER_H
