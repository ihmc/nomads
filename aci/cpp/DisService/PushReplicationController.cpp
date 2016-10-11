/*
 * PushReplicationController.cpp
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

#include "PushReplicationController.h"

#include "DisseminationService.h"
#include "DisServiceDataCacheQuery.h"
#include "DisServiceMsg.h"
#include "WorldState.h"
#include "StorageInterface.h"
#include "Logger.h"
#include "NLFLib.h"
#include "StrClass.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_ACI;
using namespace NOMADSUtil;

PushReplicationController::PushReplicationController (DisseminationService *pDisService, bool bRequireAck)
    : DataCacheReplicationController (DCRC_Push, pDisService, bRequireAck), _cv (&_m)
{
}

PushReplicationController::~PushReplicationController()
{
}

void PushReplicationController::newNeighbor (const char *pszNodeUUID, const char *pszPeerRemoteAddr,
                                             const char *pszIncomingInterface)
{
    checkAndLogMsg ("PushReplicationController::newPeer", Logger::L_Info, "the new peer is: <%s>\n", pszNodeUUID);
    DisServiceDataCacheQuery *pQuery = new DisServiceDataCacheQuery();

    pQuery->selectPrimaryKey();
    PtrLList<MessageHeader> *pMsgInfos = lockAndQueryDataCache (pQuery);
    if (pMsgInfos) {
        for (MessageHeader *pMITmp = pMsgInfos->getFirst(); pMITmp; pMITmp = pMsgInfos->getNext()) {
            replicateMessage (pMITmp, pszNodeUUID, 2000);
        }
        //replicateMessage (pMsgInfos, pszNodeUUID, 2000);
    }
    delete pMsgInfos;
    releaseQueryResults();
}

void PushReplicationController::deadNeighbor (const char *pszNodeUUID)
{
    checkAndLogMsg ("PushReplicationController::deadPeer", Logger::L_Info, "the dead peer is: <%s>\n", pszNodeUUID);
}

void PushReplicationController::newLinkToNeighbor (const char *pszNodeUID,
                                                   const char *pszPeerRemoteAddr,
                                                   const char *pszIncomingInterface)
{
}

void PushReplicationController::droppedLinkToNeighbor (const char *pszNodeUID,
                                                       const char *pszPeerRemoteAddr)
{
}

void PushReplicationController::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
}

void PushReplicationController::dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad)
{
     checkAndLogMsg ("PushReplicationController::dataCacheUpdated", Logger::L_Info, "the data cache has been updated with .\n");
    _i64LastCacheUpdate = getTimeInMilliseconds ();
}

void PushReplicationController::disServiceControllerMsgArrived (ControllerToControllerMsg *pCtrlMsg)
{
    // TODO
}

void PushReplicationController::disServiceControlMsgArrived (DisServiceCtrlMsg *pCtrlMsg)
{
}

void PushReplicationController::disServiceDataMsgArrived (DisServiceDataMsg *pDataMsg)
{
}
