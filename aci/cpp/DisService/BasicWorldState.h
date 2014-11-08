/*
 * BasicWorldState.h
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
 *
 * BasicWorldState is the simplest object that stores
 * data on the state of the network, and provides methods
 * to keep it up to date.
 */

#ifndef INCL_BASIC_WORLDSTATE_H
#define INCL_BASIC_WORLDSTATE_H

#include "PeerState.h"

#include "ConfigFileReader.h"

#include "FTypes.h"
#include "Reader.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class DisseminationService;
    class DisServiceWorldStateMsg;
    class LocalNodeInfo;
    class MessageInfo;
    class RemoteNodeInfo;
}

namespace IHMC_ACI
{
    class BasicWorldState : public PeerState
    {
        public:
            BasicWorldState (DisseminationService *pDisService);
            virtual ~BasicWorldState (void);

            void setDeadPeerInterval (uint32 ui32DeadPeerInterval);

            void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                     DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress);

            /**
             * Return the current version of the state of
             * DataCache/Topology/Subscription
             */
            int getDataCacheStateSeqId (void);
            int getTopologyStateSeqId (void);
            int getSubscriptionStateSeqId (void);

            /**
             * Returns the IP address of the node identified by pszSenderNodeId
             */
            const char * getIPAddressAsString (const char * pszSenderNodeId);
            uint32 getIPAddress (const char * pszSenderNodeId);

            /**
             * Returns the number of currently active neighbors
             */
            uint16 getNumberOfActiveNeighbors (void);

            /**
             * Increment the current version of the state of
             * DataCache/Topology/Subscription
             */
            int incrementDataCacheStateSeqId (void);
            int incrementTopologyStateSeqId (void);
            int incrementSubscriptionStateSeqId (void);

            /**
             * Set the IP address for the node identified by pszSenderNodeId
             */
            void setIPAddress (const char *pszSenderNodeId, const char *pszIPAddress);
            void setIPAddress (const char *pszSenderNodeId, uint32 ui32IPAddress);

            /**
             * Returns true if the specified node is or was ever a neighbor
             * (even if it is currently inactive)
             */
            bool wasNeighbor (const char *pszNeighborNodeId);

            /**
             * Returns true if the specified node is an active neighbor
             * (i.e., have heard from it recently)
             */
            bool isActiveNeighbor (const char *pszNeighborNodeId);

            /**
             * Returns true if the latest version of pszNeighborNodeId's state
             * received is the pszNeighborNodeId's current one
             */
            bool isNeighborUpToDate (const char *pszNeighborNodeId, uint32 ui32CurrentWorldStateSeqId);

            /**
             * NEIGHBOR STATE:
             */
            int sendKeepAliveMsg (uint8 ui8RepCtrlType, uint8 ui8FwdCtrlType);
            int updateNeighbors (void);

        private:
            friend class ForwardingController;

            int addOrActivateNeighbor (const char *pszNeighborNodeId, uint32 ui32IPAddress);

        private:
            struct NeighborState
            {
                NeighborState (void);
                NeighborState (uint32 ui32WorldStateSeqId);
                ~NeighborState (void);

                bool isUpToDate (uint32 ui32CurrentWorldStateSeqIdReceived);
                bool isStillAlive (uint32 ui32DeadPeerInterval);

                uint32 ui32ExpectedWorldStateSeqId;
                int64 i64MostRecentMessageRcvdTime;
                RemoteNodeInfo *pRNI;
            };

            bool _bDataCacheStateSeqId;
            bool _bTopologyStateSeqId;
            bool _bSubscriptionStateSeqId;

            uint32 _ui32DataCacheStateSeqId;
            uint32 _ui32TopologyStateSeqId;
            uint32 _ui32SubscriptionStateSeqId;

            uint32 _ui32DeadPeerInterval;

            NOMADSUtil::Mutex _m;            
            NOMADSUtil::StringHashtable<RemoteNodeInfo> _remoteNodes;       // Key is nodeId
            NOMADSUtil::StringHashtable<NeighborState> _neighborStates;     // Key is nodeId

            LocalNodeInfo *_pLocalNodeInfo;
            DisseminationService *_pDisService;
    }; 
}

#endif   // INCL_BASIC_WORLDSTATE_H
