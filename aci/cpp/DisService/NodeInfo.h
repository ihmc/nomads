/*
 * NodeInfo.h
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
 * NodeInfo stores state information of the local node.
 * RemoteNodeInfo stores state information of a remote node.
 */

#ifndef INCL_NODE_INFO_H
#define INCL_NODE_INFO_H

#include "ConnectivityHistory.h"
#include "ListenerNotifier.h"
#include "SubscriptionList.h"

#include "DArray.h"
#include "FTypes.h"
#include "PtrLList.h"
#include "StrClass.h"
#include "PtrLList.h"
#include "StringHashtable.h"
#include "StringHashthing.h"
#include "UInt32Hashtable.h"

#include "StringStringHashtable.h"
#include "StringFloatHashtable.h"

namespace NOMADSUtil
{
    class Graph;
    class InetAddr;
    class NICInfo;
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class History;
    class Message;
}

namespace IHMC_ACI
{
    class NodeInfo : public NOMADSUtil::StringHashthing
    {
        public:
            /**
             * NOTE: the constructor makes a copy of pszNodeId
             */
            NodeInfo (const char *pszNodeId, uint8 ui8MemorySpace = 0,
                      uint8 ui8Bandwidth = 0, int64 i64WinSize = 0);
            virtual ~NodeInfo (void);

            /**
             * Returns true if this is a new IP address for pszIPAddress, false
             * otherwise
             */
            bool setIPAddress (const char *pszIPAddress);
            bool setIPAddress (uint32 ui32IpAddress);

            const char * getNodeId (void) const;

            uint8 getMemorySpace (void) const;
            uint8 getBandwidth (void) const;
            void setMemorySpace (uint8 ui8MemorySpace);
            void setBandwidth (uint8 ui8Bandwidth);

            uint32 getDefaultIPAddress (void);

            /**
             * NOTE: the returned string MUST NOT be deallocated. 
             */
            const char * getDefaultIPAddressAsString (void);

            /**
             * Returns a null-terminated array of strings the are the ip adresses
             * associated with the peer in dotted form.
             * 
             * NOTE: the array contains references to the ip addresses; they
             *       MUST NOT be deallocated
             */
            const char ** getIPAddressesAsStrings (void);

            // Methods to manage subscriptions
            virtual NOMADSUtil::PtrLList<NOMADSUtil::String> * getAllSubscribedGroups (void) = 0;

            // Inherited methods
            virtual void setGraph (NOMADSUtil::Graph *pGraph);

            virtual NOMADSUtil::Graph * getGraph (void);
            virtual const char * getId (void);

            virtual NOMADSUtil::Thing * put (const char * pszId, NOMADSUtil::Thing *pThing);

            virtual bool contains (const char * pszKey);
            virtual NOMADSUtil::Thing * getThing (const char *pszKey);
            virtual NOMADSUtil::Thing * getParent (void);

            virtual bool isReachable (const char *pszKey);

            /**
             * Returns true if the node is reachable through any of the interfaces
             * specified in ppszInterfaces, through the interfaces specified in
             * ppNICInfos
             */
            virtual bool isReachable (const char **ppszInterfaces, NOMADSUtil::NICInfo **ppNICInfos);
            virtual bool isReachable (NOMADSUtil::PtrLList<NOMADSUtil::String> *pList, const char *pszKey);

            virtual NOMADSUtil::Thing * remove (const char * pszKey);

            virtual NOMADSUtil::PtrLList<NOMADSUtil::Thing> * list (void);
            virtual NOMADSUtil::StringHashtable<NOMADSUtil::Thing>::Iterator iterator (void);

            virtual NOMADSUtil::Thing * clone (void);
            virtual NOMADSUtil::Thing * deepClone (void);

            virtual double getWeight (void);

            virtual int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            virtual int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            virtual void setNodeImportance (uint8 ui8NodeImportance);
            virtual uint8 getNodeImportance (void);

            /**
             * Returns true if the node has importance greater that ui8Importance
             */
            bool moreImportantThan (uint8 ui8Importance);

        protected:
            struct InetAddrWrapper
            {
                InetAddrWrapper (const char *pszIfaceAddr);
                ~InetAddrWrapper (void);

                static InetAddrWrapper * getInetAddrWrapper (uint32 ui32Ip4IfaceAddr);

                bool operator > (const InetAddrWrapper &rhsIfaceWr);
                bool operator == (const InetAddrWrapper &rhsIfaceWr);
                bool operator < (const InetAddrWrapper &rhsIfaceWr);

                int64 i64MostRecentMessageRcvdTime;
                const NOMADSUtil::String ifaceAddr;
            };

            uint8 _ui8MemorySpace;
            uint8 _ui8Bandwidth;
            uint8 _ui8NodeImportance;
            const NOMADSUtil::String _nodeId;
            NOMADSUtil::PtrLList<InetAddrWrapper> _ipAddresses;
            ConnectivityHistory _connectivityHistory;
    };

    class RemoteNodeInfo : public NodeInfo
    {
        public:
            /**
             * NOTE: the constructor makes a copy of pszNodeId
             */
            RemoteNodeInfo (const char *pszNodeId, uint32 ui32CurrentTopologyStateSeqId = 0,
                            uint16 ui16CurrentSubscriptionStateCRC = 0, uint32 ui32CurrentDataCacheStateSeqId = 0);
            RemoteNodeInfo (const char *pszNodeId, SubscriptionList *pRemoteSubscriptions);
            virtual ~RemoteNodeInfo (void);

            /**
             * Methods to manage the remote node state
             */
            void setExpectedTopologyStateSeqId (uint32 ui32CurrentTopologyStateSeqId);
            void setExpectedSubscriptionStateCRC (uint16 ui16CurrentSubscriptionStateCRC);
            void setExpectedDataCacheStateSeqId (uint32 ui32CurrentDataCacheStateSeqId);

            /**
             * Returns true if this is a new IP address for pszIPAddress, false
             * otherwise
             */
            bool setMostRecentMessageRcvdTime (const char *pszRemoteAddr);

            void setTopologyStateEnabled (bool bTopologyStateEnabled);
            void incrementOccurrence (void);
            uint8 getOccurrence (void);
            uint32 getExpectedTopologyStateSeqId (void);
            uint16 getExpectedSubscriptionStateCRC (void);
            uint32 getExpectedDataCacheStateSeqId (void);
            int64 getMostRecentMessageRcvdTime (void);
            bool isTopologyStateEnabled (void) const;
            bool isNodeConfigurationEnabled (void);

            int getAndRemoveLinksToDrop (uint32 ui32DeadPeerIntervalTime,
                                         NOMADSUtil::DArray2<NOMADSUtil::String> &linksToDrop);

            /**
            * Methods to manage the remote node configuration
            */
            void setNumberOfActiveNeighbors (uint8 ui8NumberOfActiveNeighbors);
            void setNodesInConnectivityHistory (uint8 ui8ConnectionHistoryNodesCount);
            void setNodesRepetitivity (uint8 ui8NodesRepetitivity);
            uint8 getNumberOfActiveNeighbors (void);
            uint8 getNodesInConnectivityHistory (void);
            uint8 getNodesRepetitivity (void);

            /**
            * Methods to manage the remote subscriptions
            */
            NOMADSUtil::StringHashtable<uint32> * getRemoteSubscriptionStateTable (void);
            void setRemoteSubscriptionStateTable (NOMADSUtil::StringHashtable<uint32> *pSubscriptionStateTable);
            int subscribe (const char *pszGroupName, Subscription *pSubscription);
            bool hasSubscription (const char *pszGroupName);
            int unsubscribe (const char *pszGroupName);
            Subscription * getSubscription (const char *pszGroupName);
            NOMADSUtil::PtrLList<Subscription> * getSubscriptionWild (const char *pszTemplate);
            void printAllSubscribedGroups (void);
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getAllSubscribedGroups (void);
            int unsubscribeAll (void);
            SubscriptionList * getRemoteSubscriptions (void);
            SubscriptionList * getRemoteSubscriptionsCopy (void);
            bool isNodeInterested (DisServiceDataMsg *pDSDMsg);
            void printRemoteNodeInfo (void);
            
            /**
             * Topology 
             */
            NOMADSUtil::StringFloatHashtable * getIndirectProbabilities (void);
            void setIndirectProbabilities (NOMADSUtil::StringFloatHashtable * pIndirectProbabilities);
            void ageIndirectProbabilities (float fAgeParam, float fProbThreshold, int iTimeSinceLastAging);
            void printIndirectProbabilities (void);
            void addIndirectProbability (const char *pszNeighborNodeId, float fIndProb, float fAddParam, float fProbThreshold);
                   
        protected:
            friend class WorldState;
            friend class TopologyWorldState;

            bool _bTopologyStateEnabled;
            uint8 _ui8NumberOfActiveNeighbors;
            uint8 _ui8NodesInConnectivityHistory;
            uint8 _ui8NodesRepetitivity;
            uint8 _ui8NodeOccurrence;
            uint32 _ui32ExpectedTopologyStateSeqId;
            uint16 _ui16ExpectedSubscriptionStateCRC;
            uint32 _ui32ExpectedDataCacheStateSeqId;
            int64 _i64MostRecentMessageRcvdTime;

            NOMADSUtil::StringHashtable<uint32> *_pSubscriptionStateTable;
            SubscriptionList *_pRemoteSubscriptions;
            
            NOMADSUtil::StringFloatHashtable * _pIndirectProbabilities;
    };

    class LocalNodeInfo : public NodeInfo
    {
        public:
            /**
             * NOTE: the constructor makes a copy of pszNodeId
             */
            LocalNodeInfo (const char *pszNodeId, uint8 ui8MemorySpace = 0, uint8 ui8Bandwidth = 0, int64 i64WinSize = 0);
            virtual ~LocalNodeInfo (void);

            int deregisterAllGroupMembershipListeners (void);
            int deregisterGroupMembershipListener (unsigned int uiIndex);
            int registerGroupMembershipListener (GroupMembershipListener *pListener);

            int addHistory (uint16 ui16ClientId, const char *pszGroupName, History *pHistory);
            int addHistory (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, History *pHistory);
            int addHistory (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8PredicateType, const char *pszPredicate, History *pHistory);

            NOMADSUtil::PtrLList<HistoryRequest> * getHistoryRequests (uint16 ui16ClientId);

            /**
             * Returns true if the message is in the history
             */
            bool isInAnyHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender);
            bool isInHistory (uint16 ui16ClientId, Message *pMsg, uint32 ui32LatestMsgRcvdPerSender);

            // Add the specified subscription to the list of subscriptions on this local node
            // NOTE: Do not delete pSubscription regardless of whether this method succeeds or fails
            // Returns 0 if successful or a negative value in case of error
            int subscribe (uint16 ui16ClientId, const char *pszGroupName, Subscription *pSubscription);

            int addFilter (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag);
            int modifyGroupPriority (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8NewPriority);

            int removeFilter (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag);
            int removeAllFilters (uint16 ui16ClientId, const char *pszGroupName);

            int unsubscribe (uint16 ui16ClientId, const char *pszGroupName);
            int unsubscribe (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag);

            // Returns all the Subscribed groups including the On Demand groups
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getAllSubscribedGroups (void);

            // Returns all the Subscribed groups without the On Demand groups
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getAllSubscriptions (void);

            // Returns the next sequence number that will be assigned for a message pushed to this group by this node
            /*!!*/ // Bad name - rename this method to reflect the semantics of this method.
            uint32 getGroupPubState (const char *pszGroupName);

            /**
             * It returns a DArray with the IDs of the clients interested in
             * receiving the message.
             *
             * NOTE: it supports wildcards
             *
             * NOTE: if Subscription is a PREDICATE type, only the groupName is
             *       matched against the passed MessageInfo's groupName. But the
             *       predicate matching is NOT performed (this is an ack)
             */
            NOMADSUtil::DArray<uint16> * getSubscribingClients (Message *pMsg);
            NOMADSUtil::DArray<uint16> * getAllSubscribingClients (void);

            /**
             * It returns every subscription to a group matching pszGroupName
             *
             * NOTE: it supports wildcards
             */
            NOMADSUtil::PtrLList<Subscription> * getSubscriptions (const char *pszGroupName);

            /**
             * It returns theSubscriptionList associated to the ui16ClientId client
             *
             * NOTE: calling getSubscriptionListForClient() locks LocalNodeInfo
             * until releaseLocalNodeInfo() it is explicitly released by
             * calling releaseLocalNodeInfo()!!!
             */
            SubscriptionList * getSubscriptionListForClient (uint16 ui16ClientId);

            void releaseLocalNodeInfo (void);

            void removeClient (uint16 ui16ClientId);

            /**
             * Supports wildcards
             */
            bool hasSubscription (Message *pMessage);
            bool hasSubscription (uint16 ui16ClientId, Message *pMessage);

            // Checks to see if any client has subscribed to the specified group
            // (and tag) and requested reliable delivery
            bool requireGroupReliability (const char *pszGroupName, uint16 ui16Tag);
            bool requireMessageReliability (const char *pszGroupName, uint16 ui16Tag);
            bool requireSequentiality (const char *pszGroupName, uint16 ui16Tag);

            int incrementGroupPubState (const char *pszGroupName);
            int setGroupPubState (const char *pszGroupName, uint32 ui32NewSeqId);

            int subscriptionListUpdated (void);

            void addNodeToConnectivityHistory (const char *pszNodeId);
            void setConnectivityHistoryWinSize (int64 i64WinSize);
            uint8 getConnectivityHistoryNodesCount (void);

            uint32 getSubscriptionStateSequenceID (void) const;
            
            SubscriptionList * getConsolidatedSubscriptions (void);
            SubscriptionList * getConsolidatedSubscriptionsCopy (void);
            void printAllSubscribedGroups (void);

        private:
            int addHistoryInternal (uint16 ui16ClientId, const char *pszGroupName, History *pHistory);
            Subscription * getSubscriptionForClient (uint16 ui16ClientId, const char *pszGroupName);
            void updateConsolidateSubsciptionList (const char *pszGroupName, Subscription *pSubscription);
            void recomputeConsolidateSubsciptionList (void);

            /*
             * For every client in _localSubscriptions - if it has the Subscription
             * to the specified group - add its filters on the subscription if and only
             * if all the other clients subscribing the group filters the considered tag.
             */
            void addAddFiltersToConsolidateList (const char *pszGroupName);

        private:
            struct GroupPubState
            {
                GroupPubState (void);
                uint32 ui32NextSeqNum;
            };

            uint32 ui32SubscriptionStateSeqID;  // Incremented any time the _pConsolidatedSubscriptions
                                                // changes its content
            SubscriptionList _consolidatedSubscriptions; // Aggregate list that contains all the
                                                         // subscription for each client on the node
            NOMADSUtil::LoggingMutex _m;
            NOMADSUtil::UInt32Hashtable<SubscriptionList> _localSubscriptions;
            NOMADSUtil::StringHashtable<GroupPubState> _pubState;    // Stores information about local node's publications on a per-group basis
            GroupMembershipListenerNotifier _notifier;
    };

    //==========================================================================
    // NodeInfo
    //==========================================================================

    inline uint8 NodeInfo::getMemorySpace() const
    {
        return _ui8MemorySpace;
    }

    inline uint8 NodeInfo::getBandwidth() const
    {
        return _ui8Bandwidth;
    }

    inline void NodeInfo::setMemorySpace(uint8 ui8MemorySpace)
    {
        _ui8MemorySpace = ui8MemorySpace;
    }

    inline void NodeInfo::setBandwidth(uint8 ui8Bandwidth)
    {
        _ui8Bandwidth = ui8Bandwidth;
    }

    //==========================================================================
    // RemoteNodeInfo
    //==========================================================================

    inline void RemoteNodeInfo::setNumberOfActiveNeighbors (uint8 ui8NumberOfActiveNeighbors)
    {
        _ui8NumberOfActiveNeighbors = ui8NumberOfActiveNeighbors;
    }

    inline void RemoteNodeInfo::setNodesInConnectivityHistory (uint8 ui8ConnectionHistoryNodesCount)
    {
        _ui8NodesInConnectivityHistory = ui8ConnectionHistoryNodesCount;
    }

    inline void RemoteNodeInfo::setNodesRepetitivity (uint8 ui8NodesRepetitivity)
    {
        _ui8NodesRepetitivity = ui8NodesRepetitivity;
    }

    inline uint8 RemoteNodeInfo::getNumberOfActiveNeighbors (void)
    {
        return _ui8NumberOfActiveNeighbors;
    }

    inline uint8 RemoteNodeInfo::getNodesInConnectivityHistory (void)
    {
        return _ui8NodesInConnectivityHistory;
    }

    inline uint8 RemoteNodeInfo::getNodesRepetitivity (void)
    {
        return _ui8NodesRepetitivity;
    }

    //==========================================================================
    // InetAddrWrapper
    //==========================================================================

    inline NodeInfo::InetAddrWrapper::InetAddrWrapper (const char *pszIfaceAddr)
        : ifaceAddr (pszIfaceAddr)
    {
    }

    inline NodeInfo::InetAddrWrapper::~InetAddrWrapper()
    {
    }

    inline NodeInfo::InetAddrWrapper * NodeInfo::InetAddrWrapper::getInetAddrWrapper (uint32 ui32Ip4IfaceAddr)
    {
        NOMADSUtil::InetAddr addr (ui32Ip4IfaceAddr);
        return new NodeInfo::InetAddrWrapper (addr.getIPAsString());
    }

    inline bool NodeInfo::InetAddrWrapper::operator > (const InetAddrWrapper &rhsIfaceWr)
    {
        return (i64MostRecentMessageRcvdTime > rhsIfaceWr.i64MostRecentMessageRcvdTime);
    }

    inline bool NodeInfo::InetAddrWrapper::operator == (const InetAddrWrapper &rhsIfaceWr)
    {
        return ((ifaceAddr == rhsIfaceWr.ifaceAddr) == 1);
    }

    inline bool NodeInfo::InetAddrWrapper::operator < (const InetAddrWrapper &rhsIfaceWr)
    {
        return (i64MostRecentMessageRcvdTime < rhsIfaceWr.i64MostRecentMessageRcvdTime);
    }

    //==========================================================================
    // GroupPubState
    //==========================================================================

    inline LocalNodeInfo::GroupPubState::GroupPubState (void)
    {
        ui32NextSeqNum = 0;
    } 
}

#endif   // #ifndef INCL_NODE_INFO_H
