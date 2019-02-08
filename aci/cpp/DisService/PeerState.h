/**
 * PeerState.h
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
 * Data structure to keep track of state information
 * of the peers of the network.
 *
 * NOTE:
 * - Peer is any node in the network.
 * - Neighbor is a peer at 1-hop distance.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 31, 2011, 11:54 AM
 */

#ifndef INCL_PEER_STATE_H
#define	INCL_PEER_STATE_H

#include "Controllable.h"

#include "ListenerNotifier.h"

#include "StringHashtable.h"

namespace NOMADSUtil
{
    template <class T> class StringHashtable;
}

namespace IHMC_ACI
{
    class DisServiceCtrlMsg;
    class DisseminationService;
    class LocalNodeInfo;
    class RemoteNodeInfo;

    class PeerState : public Controllable, public MessageListener
    {
        public:
            enum NodeParametersValues {
                VERY_LOW = 0x00,
                LOW = 0x01,
                NORMAL = 0x02,
                HIGH = 0x03,
                VERY_HIGH = 0x04,
            };

            enum Type {
                NEIGHBOR_STATES = 0x01,
                TOPOLOGY_STATE = 0x02,
                IMPROVED_TOPOLOGY_STATE = 0x03
            };

            virtual ~PeerState (void);

            static PeerState * getInstance (DisseminationService *pDisService, Type type);
            static void registerWithListeners (DisseminationService *pDisService, PeerState *pPeerState);

            // Notifier
            int deregisterAllPeerStateListeners (void);
            int deregisterPeerStateListener (unsigned int uiIndex);
            int registerPeerStateListener (PeerStateListener *pListener);

            // PeerState Configuration
            virtual void setDeadPeerInterval (uint32 ui32DeadPeerInterval);
            virtual void setLocalNodeInfo (LocalNodeInfo *pLocalNodeInfo);

            Type getType (void);

            virtual DisServiceCtrlMsg * getKeepAliveMsg (uint8 ui8RepCtrlType, uint8 ui8FwdCtrlType, uint8 ui8NodeImportance) = 0;

            /**
             * Returns the number of currently active neighbors. (By active "neighbor"
             * it is meant any peer that is at a 1-hop distance)
             */
            virtual uint16 getNumberOfActiveNeighbors (void) = 0;

            virtual uint16 getNumberOfActiveNeighborsWithImportance (uint8 ui8Importance, bool *bNodesWithHigherImportance) = 0;

            /**
             * Returns the number of currently active peers. (By active "peer" it is
             * meant any node that is reachable in any number)
             */
            virtual uint16 getNumberOfActivePeers (void) = 0;

            /**
             * Returns a pointer to a RemoteNodeInfo object that contains the state
             * information of the peer identified by pszNodeId.
             *
             * NOTE: lock and unlock before retrieving the RemoteNodeInfo.
             */
            virtual RemoteNodeInfo * getPeerNodeInfo (const char *pszNodeId) = 0;

            /**
             * Returns an iterator to iterate through the list of all the neighbors/peers
             * for which the node collected state information.
             * The list of the peers, contain the neighbors as well.
             *
             * NOTE: lock and unlock before retrieving the RemoteNodeInfo.
             * NOTE: the list must be deallocated using the appropriate release()
             *       method
             */
            virtual RemoteNodeInfo ** getAllNeighborNodeInfos (void) = 0;
            virtual void release (RemoteNodeInfo **ppRNIs) = 0;

            virtual NOMADSUtil::StringHashtable<RemoteNodeInfo> * getAllPeerNodeInfos (void) = 0;
            virtual void release (NOMADSUtil::StringHashtable<RemoteNodeInfo> *pPtrLList) = 0;

            virtual char ** getAllNeighborNodeIds (void) = 0;
            virtual void release (char **ppszIDs) = 0;

            /**
             * Returns true if the specified node is an active neighbor (i.e., have
             * heard from it recently)
             */
            virtual bool isActiveNeighbor (const char *pszNeighborNodeId) = 0;

            /**
             * Returns true if the latest version of pszNeighborNodeId's state
             * received is the pszNeighborNodeId's current one
             */
            virtual bool isPeerUpToDate (const char *pszNeighborNodeId, uint16 ui16CurrentSubscriptionStateCRC) = 0;

            virtual bool wasNeighbor (const char *pszNeighborNodeId) = 0;

            /*
             * Set/Get IP Address(es)
             */
            int setIPAddress (const char *pszSenderNodeId, const char *pszIPAddress);
            int setIPAddress (const char *pszSenderNodeId, uint32 ui32IPAddress);

            uint32 getIPAddress (const char *pszSenderNodeId);

            const char * getIPAddressAsString (const char *pszSenderNodeId);
            const char ** getIPAddressesAsStrings (const char *pszSenderNodeId);

            virtual char ** getAllNeighborIPs (void) = 0;
            const char * getSenderNodeIdByIPddress (uint32 ui32Address);

            /**
             * Iterates through the list of the active neighbors to find the
             * inactive ones.
             *
             * NOTE: this method should be periodically called.
             */
            virtual int updateNeighbors (void) = 0;

        protected:
            PeerState (Type type);

        protected:
            uint32 _ui32DeadPeerInterval;
            LocalNodeInfo *_pLocalNodeInfo;
            PeerStateListenerNotifier _notifier;

        protected:
            Type _type;
    };
}

#endif	// INCL_PEER_STATE_H

