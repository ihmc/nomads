/*
 * PullReplicationController.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on January 23, 2009, 4:55 PM
 */

#ifndef INCL_PULL_REPLICATION_CONTROLLER_H
#define	INCL_PULL_REPLICATION_CONTROLLER_H

#include "DataCacheReplicationController.h"

#include "DisServiceDataCacheQuery.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class String;
}

namespace IHMC_ACI
{
    /**
     * Simple DataCacheReplicationController implementation with a PULLER behaviour.
     *
     * PullReplicationController keeps track of the last time it received replicas.
     *
     */
    class PullReplicationController : public DataCacheReplicationController,
                                      public GroupMembershipService,
                                      public NOMADSUtil::ManageableThread
    {
        public:
            PullReplicationController (DisseminationService *pDisService,
                                       bool bRequireAck=DataCacheReplicationController::DEFAULT_REQUIRE_ACK);
            virtual ~PullReplicationController (void);

            void run (void);

            virtual void newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                      const char *pszIncomingInterface);
            virtual void deadNeighbor (const char *pszNodeUUID);
            virtual void newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                            const char *pszIncomingInterface);
            virtual void droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr);
            virtual void stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate);
            virtual void dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad);

        protected:
            virtual void disServiceControllerMsgArrived (ControllerToControllerMsg *pCtrlMsg);
            virtual void disServiceControlMsgArrived (DisServiceCtrlMsg *pCtrlMsg);
            virtual void disServiceDataMsgArrived (DisServiceDataMsg *pDataMsg);

        private:
            NOMADSUtil::String * getCondition (const char * pszGroupName);

            void addToReplyingPeers(const char * pszNodeId);
            void addToUnreplyingPeers(const char * pszNodeId);
            void updateUnreplyingPeers (void);

            int64 _ui64LastReplication;
            uint16 _ui16DataCacheQueryMsgTimer;

            DisServiceDataCacheQuery _query;
            DisseminationService *_pDisService;

            NOMADSUtil::LoggingMutex _mReplyingPeersQueue;
            NOMADSUtil::PtrLList<NOMADSUtil::String> _cacheQueryReplyingPeers;
            NOMADSUtil::PtrLList<NOMADSUtil::String> _cacheQueryUnreplyingPeers;
    };
}

#endif  // INCL_PULL_REPLICATION_CONTROLLER_H
