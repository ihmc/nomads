/*
 * WorldState.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#ifndef INCL_WORLDSTATE_H
#define INCL_WORLDSTATE_H

#include "PeerState.h"

#include "DisseminationService.h"
#include "ForwardingController.h"
#include "Listener.h"
#include "NodeInfo.h"
#include "Subscription.h"
#include "SubscriptionList.h"

#include "CRC.h"
#include "FTypes.h"
#include "StringHashgraph.h"
#include "PtrLList.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    class InstrumentedWriter;
    class Reader;
}

namespace IHMC_ACI
{
    class DisseminationService;
    class DisServiceWorldStateSeqIdMsg;
    class DisServiceWorldStateMsg;
    class MessageInfo;
    class RemoteNodeInfo;

    class WorldState : public PeerState, public GroupMembershipListener
    {
        public:
            WorldState (DisseminationService *pDisService);
            virtual ~WorldState (void);

            void setLocalNodeInfo (LocalNodeInfo *pLocalNodeInfo);

            // Add Neighbor if is new or if is a dead peer
            virtual void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, DisServiceMsg *pDisServiceMsg,
                                     uint32 ui32SourceIPAddress, const char *pszIncomingInterfaces);

            DisServiceCtrlMsg * getKeepAliveMsg (uint8 ui8RepCtrlType, uint8 ui8FwdCtrlType, uint8 ui8NodeImportance);
            // Returns true if the specified node is or was ever a neighbor (even if it is currently inactive)
            bool wasNeighbor (const char *pszNeighborNodeId);
            // Returns true if the specified node is an active neighbor (i.e., have heard from it recently)
            bool isActiveNeighbor (const char *pszNeighborNodeI);

            virtual int updateNeighbors (void);

            RemoteNodeInfo * getNeighborNodeInfo (const char *pszNodeId);

            RemoteNodeInfo ** getAllNeighborNodeInfos (void);
            void release (RemoteNodeInfo **ppRNIs);

            NOMADSUtil::StringHashtable<RemoteNodeInfo> * getAllPeerNodeInfos (void);
            void release (NOMADSUtil::StringHashtable<RemoteNodeInfo> *pPtrLList);

            RemoteNodeInfo * getPeerNodeInfo (const char *pszNodeId);

            char ** getAllNeighborNodeIds (void);
            char ** getAllNeighborIPs (void);
            void release (char **ppszIDs);

            bool isPeerUpToDate (const char *pszNeighborNodeId, uint16 ui16CurrentSubscriptionStateCRC);

            /*
             * NODE CONFIGURATION / CONNECTION METHODS
             */
            void newSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription);
            void removedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription);
            void modifiedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription);

            void incrementTopologyStateSeqId (void);

            void incrementDataCacheStateSeqId (void);
            int setNodeControllerTypes (const char *pszNeighborNodeId, uint8 ui8RepCtrlType, uint8 ui8FwdCtrlType);
            int updateNeighborConfiguration (const char * pszNeighborNodeId, uint8 ui8NumberOfActiveNeighbors,
                                             uint8 ui8MemorySpace,uint8 ui8Bandwidth, uint8 ui8NodesInConnectivityHistory,
                                             uint8 ui8NodesRepetitivity);
            uint32 getTopologyStateSeqId (void);
            uint16 getSubscriptionStateCRC (void);
            uint32 getDataCacheStateSeqId (void);
            uint8 getRepControllerType (const char *pszNeighborNodeId);
            uint8 getFwdControllerType (const char *pszNeighborNodeId);
            uint16 getNumberOfActiveNeighbors (void);
            uint16 getNumberOfActiveNeighborsWithImportance (uint8 ui8Importance, bool *bNodesWithHigherImportance);
            uint16 getNumberOfActivePeers (void);
            uint8 getMemorySpace (const char *pszNeighborNodeId);
            uint8 getBandwidth (const char *pszNeighborNodeId);
            uint8 getNodesInConnectivityHistory (const char * pszNeighborNodeId);
            // Return  the probability that certain node occurs based on the repetitiveness
            // NB return VERY_LOW only if the occurrence of a node is 0
            uint8 getNodeOccurrency (const char *pszNeighborNodeId);
            // Return the probability that I am on a repetitive path
            uint8 getRepetitiveness (void);
            int setNodeImportance (const char *pszNeighborNodeId, uint8 ui8NodeImportance);
            uint8 getNodeImportance (const char *pszNeighborNodeId);

            /*
             * TOPOLOGY STATE
             */

            /*
             * SUBSCRIPTION STATE
             */
            // TODO subscription with tag and predicate
            // unsubscribe
            virtual int sendSubscriptionStateMsg (const char *pszNodeId);
            virtual int sendSubscriptionStateMsg (NOMADSUtil::PtrLList<NOMADSUtil::String> *pSubscriptions, const char *pszNodeId, uint32 *pui32SeqId);
            virtual int sendSubscriptionState (void);
            virtual int sendSubscriptionStateReqMsg (DisServiceSubscriptionStateReqMsg *pSSReqMsg);
            // If there is a req that has to be served and the target node is the same of the received req, don't send the req
            int receivedSubscriptionStateReqMsg (DisServiceSubscriptionStateReqMsg *pSSReqMsg);
            bool isSubscriptionStateUpToDate (const char *pszNeighbprNodeId, uint16 ui16NeighborSubscriptionStateCRC);
            virtual int updateSubscriptionState (NOMADSUtil::StringHashtable<NOMADSUtil::DArray2<NOMADSUtil::String> > *pSubscriptionsTable, NOMADSUtil::StringHashtable<uint32> *pNodesTable);

            /*
             * FORWARDING
             */
            bool isDirectForwarding (const char *pszGroupName);
            // Returns the target node
            // Return NULL if don't have enough information to compute the probability
            const char * getProbTarget (const char * pszPeerNodeId);
            // Return all the neighbors that reachable only through me
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getIsolatedNeighbors (void);
            float getStatefulForwardProbability (const char * pszPeerNodeId);

            /*
            bool isReachable (const char *pszNodeId);
            const char * getNextNodeToReachTarget (const char *pszNodeId);
            */

        private:
            enum EventType {
                NewNeighbor  = 0x00,
                DeadNeighbor = 0x01,
                NewLink      = 0x02,
                Update       = 0x03
            };

            struct Event
            {
                virtual ~Event (void);

                EventType type;
                char *pszNodeId;

                protected:
                    Event (EventType eventType, char *pszEventNodeId);
            };

            struct NewNeighborEvent : public Event
            {
                NewNeighborEvent (char *pszEventNodeId);
                ~NewNeighborEvent (void);
            };

            struct DeadNeighborEvent : public Event
            {
                DeadNeighborEvent (char *pszEventNodeId);
                ~DeadNeighborEvent (void);
            };

            struct NewLinkEvent : public Event
            {
                NewLinkEvent (char *pszEventNodeId, char *pszNewLinkAddr);
                ~NewLinkEvent (void);

                char *pszNewLinkAddr;
            };

            struct UpdateEvent : public Event
            {
                UpdateEvent (char *pszEventNodeId, PeerStateListener::UInt16PeerStateUpdate *pUpdateEvent);
                ~UpdateEvent (void);
                
                PeerStateListener::UInt16PeerStateUpdate *pUpdate;
            };

            virtual int addOrActivateNeighbor (const char *pszNeighborNodeId, uint32 ui32IPAddress, Event *&pEvent);
            virtual int addOrActivateNeighbor (const char *pszNeighborNodeId, uint32 ui32IPAddress,
                                       DisServiceWorldStateSeqIdMsg *pDSWSMsg, Event *&pEvent);
            virtual int addOrActivateNeighborInternal (const char *pszNeighborNodeId,
                                               uint32 ui32IPAddress,
                                               DisServiceWorldStateSeqIdMsg *pDSWSMsg,
                                               Event *&pEvent);

            virtual void incrementSubscriptionStateSeqId (void);

        private:
            friend class ForwardingController;
            friend class TopologyWorldState;

            bool isPeerAlive (RemoteNodeInfo *pRemoteNode);
            void moveChildToDeadPeers (RemoteNodeInfo *pRemoteNode);

            /*
            // return all the node in the path to reach pszNodeId
            const char * getRicorsivePath (const char *pCurrentNodeId, const char *pszNodeId, NOMADSUtil::Graph *pMSPGraph);
            */

            DisseminationService *_pDisService;
            NOMADSUtil::Graph *_pNodesGraph;                            // Graph containing the alive remote nodes
            NOMADSUtil::StringHashtable<RemoteNodeInfo> _deadPeers;     // Contains only the dead peers
            NOMADSUtil::StringHashtable<uint32> _subscriptionStateTable;// Contains the subscriptionState's history
            NOMADSUtil::CRC _crc;

            uint32 _ui32DataCacheStateSeqId;
            uint32 _ui32TopologyStateSeqId;
            uint32 _ui32SubscriptionStateSeqId;
            uint16 _ui16SubscriptionStateCRC;

            // Variable used to calculate the repetitiveness
            float _ui32NumberOfNewPeer;
            float _ui32TotalNumberOfPeer;

            uint8 _ui8ForwardingType;

            // Queue for SubscriptionStateMg and SubscriptionStateReqMsg
            DisServiceSubscriptionStateMsg *_pSSMsg;
            DisServiceSubscriptionStateReqMsg *_pSSReqMsg;
            NOMADSUtil::Mutex _mPeerStateNotification;
    };

    // WorldState::Event

    inline WorldState::Event::Event (EventType eventType, char *pszEventNodeId)
    {
        type = eventType;
        pszNodeId = pszEventNodeId;
    }

    inline WorldState::Event::~Event (void)
    {
        free (pszNodeId);
        pszNodeId = NULL;
    }

    // WorldState::NewNeighborEvent

    inline WorldState::NewNeighborEvent::NewNeighborEvent (char *pszEventNodeId)
        : WorldState::Event (NewNeighbor, pszEventNodeId)
    {
    }

    inline WorldState::NewNeighborEvent::~NewNeighborEvent (void)
    {
    }

    // WorldState::DeadNeighborEvent

    inline WorldState::DeadNeighborEvent::DeadNeighborEvent (char *pszEventNodeId)
        : WorldState::Event (DeadNeighbor, pszEventNodeId)
    {
    }

    inline WorldState::DeadNeighborEvent::~DeadNeighborEvent (void)
    {
    }

    inline WorldState::NewLinkEvent::NewLinkEvent (char *pszEventNodeId, char *pszNewLinkAddress)
        : WorldState::Event (NewLink, pszEventNodeId)
    {
        pszNewLinkAddr = pszNewLinkAddress;
    }

    inline WorldState::NewLinkEvent::~NewLinkEvent (void)
    {
        free (pszNewLinkAddr);
        pszNewLinkAddr = NULL;
    }

    // WorldState::UpdateEvent

    inline WorldState::UpdateEvent::UpdateEvent (char *pszEventNodeId,
                                                 PeerStateListener::UInt16PeerStateUpdate *pUpdateEvent)
        : WorldState::Event (Update, pszEventNodeId)
    {
        pUpdate = pUpdateEvent;
    }

    inline WorldState::UpdateEvent::~UpdateEvent (void)
    {
        delete pUpdate;
        pUpdate = NULL;
    }
}

#endif   // #ifndef INCL_WORLDSTATE_H
