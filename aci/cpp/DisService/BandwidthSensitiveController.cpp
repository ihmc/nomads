/*
 * BandwidthSensitiveController.cpp
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

#include "BandwidthSensitiveController.h"

#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "DisServiceMsg.h"
#include "Message.h"
#include "NodeInfo.h"
#include "TransmissionService.h"

#include "Logger.h"
#include "NLFLib.h"
#include "DisseminationServiceProxyServer.h"

#include <stddef.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

BandwidthSensitiveController::BandwidthSensitiveController (DisseminationService *pDisService,
                                                            bool bRequireAck)
    : DataCacheReplicationController (DCRC_BandwidthSensitive, pDisService, bRequireAck),
      TopologyService (pDisService),
     _mData (2), _mPeers (1)
{
    _pTrHistory = getTrasmissionHistoryInterface();
    if (_pTrHistory == NULL) {
        checkAndLogMsg ("BandwidthSensitiveController::BandwidthSensitiveController",
                        Logger::L_SevereError, "_pTrHistory is NULL,  Quitting\n");
        exit (-1);
    }

    _pTrSvc = getTrasmissionService();
    if (_pTrHistory == NULL) {
        checkAndLogMsg ("BandwidthSensitiveController::BandwidthSensitiveController",
                        Logger::L_SevereError, "_pTrSvc is NULL,  Quitting\n");
        exit (-1);
    }

    _pszNodeId = strDup (pDisService->getNodeId());
}

BandwidthSensitiveController::BandwidthSensitiveController (DisseminationServiceProxyServer *pDisServiceProxy,
                                                            bool bRequireAck)
    : DataCacheReplicationController (DCRC_BandwidthSensitive, pDisServiceProxy, bRequireAck),
      TopologyService (pDisServiceProxy->getDisseminationServiceRef()),
     _mData (2), _mPeers (1)
{
    _pTrHistory = getTrasmissionHistoryInterface();
    if (_pTrHistory == NULL) {
        checkAndLogMsg ("BandwidthSensitiveController::BandwidthSensitiveController",
                        Logger::L_SevereError, "_pTrHistory is NULL,  Quitting\n");
        exit (-1);
    }

    _pTrSvc = getTrasmissionService();
    if (_pTrHistory == NULL) {
        checkAndLogMsg ("BandwidthSensitiveController::BandwidthSensitiveController",
                        Logger::L_SevereError, "_pTrSvc is NULL,  Quitting\n");
        exit (-1);
    }

    _pszNodeId = strDup (pDisServiceProxy->getDisseminationServiceRef()->getNodeId());
}

BandwidthSensitiveController::~BandwidthSensitiveController()
{
    delete _pszNodeId;
    _pszNodeId = NULL;
}

void BandwidthSensitiveController::networkQuiescent (const char **pszInterfaces)
{
    if (pszInterfaces == NULL) {
        return;
    }
    _mPeers.lock (333);
    // TODO: Fix this!
    // NICInfo **ppNICInfos = _pTrSvc->getActiveInterfaces();
    NICInfo **ppNICInfos = NULL;
    RemoteNodeInfo *pRNI;
    for (unsigned int i = 0; i < _peers.size(); i++) {
        // Get chunks
        //DataCache
        // Get peers
        pRNI = getNeighborNodeInfo ((const char *)_peers[i]);
        if (pRNI != NULL && pRNI->isReachable (pszInterfaces, ppNICInfos)) {
            Message *pMsg = NULL;
            DisServiceDataMsg msg (_pszNodeId, pMsg, (const char *)_peers[i]);

          //  if (0 != broadcastDataMessage ()) {
                // Error
          //  }
        }
    }
    _mPeers.unlock (333);
}

void BandwidthSensitiveController::dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad)
{
    _mData.lock (1);
    _bNextData = true;
    _mData.unlock (1);
}

void BandwidthSensitiveController::capacityReached (void)
{
}

void BandwidthSensitiveController::thresholdCapacityReached (uint32 ui32Length)
{
}

void BandwidthSensitiveController::spaceNeeded (uint32 ui32bytesNeeded, MessageHeader *pIncomingMgsInfo, void *pIncomingData)
{
}

void BandwidthSensitiveController::newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                                const char *pszIncomingInterface)
{
    _mPeers.lock (2);
    _peers[_peers.firstFree()] = pszNodeUID;  // String makes a copy of pszNodeUID
    _mPeers.unlock (2);
}

void BandwidthSensitiveController::deadNeighbor (const char *pszNodeUID)
{
    _mPeers.lock (3);
    for (unsigned int i = 0; i < _peers.size(); i++) {
        if (strNotNullAndEqual (pszNodeUID, (const char *)_peers[i])) {
            _peers.clear (i);
            break;
        }
    }
    _mPeers.unlock (3);
}

void BandwidthSensitiveController::newLinkToNeighbor (const char *pszNodeUID,
                                                      const char *pszPeerRemoteAddr,
                                                      const char *pszIncomingInterface)
{
}

void BandwidthSensitiveController::droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr)
{
}

void BandwidthSensitiveController::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
}

int BandwidthSensitiveController::cacheCleanCycle()
{
    return 0;
}

void BandwidthSensitiveController::disServiceControllerMsgArrived (ControllerToControllerMsg *pCtrlMsg)
{
}

void BandwidthSensitiveController::disServiceControlMsgArrived (DisServiceCtrlMsg *pCtrlMsg)
{
}

void BandwidthSensitiveController::disServiceDataMsgArrived (DisServiceDataMsg *pDataMsg)
{ 
}

bool BandwidthSensitiveController::newData()
{
    _mData.lock (4);
    bool bRet = _bNextData;
    _mData.unlock (4);
    return bRet;
}

