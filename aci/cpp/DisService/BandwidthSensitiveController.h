/*
 * BandwidthSensitiveController.h
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
 * Author: Giacomo Benincasa     (gbenincasa@ihmc.us)
 * Created on May 19, 2011, 6:00 PM
 */

#ifndef INCL_BANDWIDTH_SENSITIVE_CONTROLLER_H
#define	INCL_BANDWIDTH_SENSITIVE_CONTROLLER_H

#include "DataCacheReplicationController.h"
#include "Listener.h"
#include "TransmissionHistoryInterface.h"

#include "DArray2.h"
#include "LoggingMutex.h"
#include "StrClass.h"

namespace IHMC_ACI
{
    class DisseminationService;
    class TransmissionService;

    class BandwidthSensitiveController : public DataCacheReplicationController, public NetworkStateListener, TopologyService
    {
        public:
            BandwidthSensitiveController (DisseminationService *pDisService,
                                          bool bRequireAck = DataCacheReplicationController::DEFAULT_REQUIRE_ACK);
            BandwidthSensitiveController (DisseminationServiceProxyServer *pDisServiceProxy,
                                          bool bRequireAck = DataCacheReplicationController::DEFAULT_REQUIRE_ACK);
            virtual ~BandwidthSensitiveController (void);

            void networkQuiescent (const char **pszInterfaces);

            virtual void dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad);
            virtual void capacityReached (void);
            virtual void thresholdCapacityReached (uint32 ui32Length);
            virtual void spaceNeeded (uint32 ui32bytesNeeded, MessageHeader *pIncomingMgsInfo, void *pIncomingData);
            virtual void newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                      const char *pszIncomingInterface);
            virtual void deadNeighbor (const char *pszNodeUID);
            virtual void newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                            const char *pszIncomingInterface);
            virtual void droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr);
            virtual void stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate);
            virtual int cacheCleanCycle (void);

        protected:
            virtual void disServiceControllerMsgArrived (ControllerToControllerMsg *pCtrlMsg);
            virtual void disServiceControlMsgArrived (DisServiceCtrlMsg *pCtrlMsg);
            virtual void disServiceDataMsgArrived (DisServiceDataMsg *pDataMsg);

        private:
            bool newData (void);

        private:
            bool _bAbsThresh;
            bool _bNextData;
            uint32 _ui32AbsoluteThreshold;
            float _fPercentThrshold;

            const char *_pszNodeId;
            NOMADSUtil::LoggingMutex _mData;
            NOMADSUtil::LoggingMutex _mPeers;
            NOMADSUtil::DArray2<NOMADSUtil::String> _peers;
            TransmissionHistoryInterface *_pTrHistory;
            TransmissionService *_pTrSvc;
    };
}

#endif	// INCL_BANDWIDTH_SENSITIVE_CONTROLLER_H

