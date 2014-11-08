/*
 * PushToConvoyDataCacheReplicationController.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on January 28, 2009, 12:56 PM
 */

#ifndef INCL_PUSH_TO_CONVOY_DATA_CACHE_REPLICATION_CONTROLLER_H
#define	INCL_PUSH_TO_CONVOY_DATA_CACHE_REPLICATION_CONTROLLER_H

#include "DataCacheReplicationController.h"

#include "StringHashtable.h"

namespace IHMC_ACI
{
    /**
     * DataCacheReplicationController implementation with a PUSHER behaviour.
     *
     * PushToConvoyDataCacheReplicationController keeps track of the latest time it
     * performed a replication for every subscription.
     *
     * If PushToConvoyDataCacheReplicationController can retrieve the subscription
     * information of the new peer, it replicates all the messages matching the new
     * peer's subscriptionss since the last replication until the current time.
     *
     * If the new node's subsctiptions can not be retrieved, all the messages in
     * the cache since the last replication until the current time are replicated.
     */
    class PushToConvoyDataCacheReplicationController : public DataCacheReplicationController
    {
        public:
            PushToConvoyDataCacheReplicationController (DisseminationService *pDisService,
                                                        bool bRequireAck = DataCacheReplicationController::DEFAULT_REQUIRE_ACK);
            virtual ~PushToConvoyDataCacheReplicationController (void);

            virtual void newNeighbor (const char *pszNodeUUID, const char *pszPeerRemoteAddr,
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
            int64 _i64TimeOut;
            int64 _uint64LatestReplicationTime;
            int64 _i64LastCacheUpdate;

            NOMADSUtil::ConditionVariable _cv;
            NOMADSUtil::Mutex _m;
    };
}

#endif  // INCL_PUSH_TO_CONVOY_DATA_CACHE_REPLICATION_CONTROLLER_H
