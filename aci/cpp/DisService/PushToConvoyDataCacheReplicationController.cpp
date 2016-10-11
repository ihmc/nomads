/*
 * PushToConvoyDataCacheReplicationController.cpp
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

#include "PushToConvoyDataCacheReplicationController.h"

#include "DisServiceDataCacheQuery.h"
#include "DisServiceDefs.h"
#include "MessageInfo.h"

#include "Logger.h"
#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

PushToConvoyDataCacheReplicationController::PushToConvoyDataCacheReplicationController (DisseminationService *pDisService, bool bRequireAck)
    : DataCacheReplicationController (DCRC_PushToConvoy, pDisService, bRequireAck), _cv (&_m)
{
    _uint64LatestReplicationTime = 0;
}

PushToConvoyDataCacheReplicationController::~PushToConvoyDataCacheReplicationController ()
{
}

void PushToConvoyDataCacheReplicationController::newNeighbor (const char *pszNodeUUID,
                                                              const char *pszPeerRemoteAddr,
                                                              const char *pszIncomingInterface)
{
    checkAndLogMsg ("PushToConvoyDataCacheReplicationController::newPeer",
                    Logger::L_Info, "the new peer is: <%s>\n", pszNodeUUID);

    // Get the World State for the new neighbor
    const char * pszGroupName = NULL;

    // Create the query
    DisServiceDataCacheQuery query;
    query.selectPrimaryKey ();
    if (pszGroupName != NULL) {
        query.addConstraintOnGroupName(pszGroupName);
    }
    if (_uint64LatestReplicationTime != 0) {
        query.addConstraintOnArrivalTimeGreaterThan (_uint64LatestReplicationTime);
    }

    // Exec the query
    PtrLList<MessageHeader> *pMsgInfos = lockAndQueryDataCache (&query);

    _uint64LatestReplicationTime = getTimeInMilliseconds();
    //replicateMessage (pMsgInfos, pszNodeUUID, _i64TimeOut);
    for (MessageHeader *pMITmp = pMsgInfos->getFirst(); pMITmp; pMITmp = pMsgInfos->getNext()) {
        replicateMessage (pMITmp, pszNodeUUID, _i64TimeOut);
    }

//    if (pMsgInfos || _pAck->hasEnquequedMessages (pszNodeUUID)) {
//        // Memorize the last replication session
//        _uint64LatestReplicationTime = getTimeInMilliseconds();
//
//        // Replicate every message retrieved
//        if (_bRequireAck) {
//            _pAck->ackRequest (pszNodeUUID, pMsgInfos);
//        }
//        else {
//            _m.lock();
//            for (MessageInfo *pMI = pMsgInfos->getFirst(); pMI != NULL; pMI = pMsgInfos->getNext()) {
//
//            }
//            _m.unlock();
//        }
//    }
}

void PushToConvoyDataCacheReplicationController::deadNeighbor (const char *pszNodeUUID)
{
    checkAndLogMsg ("PushToConvoyDataCacheReplicationController::deadPeer",
                    Logger::L_Info, "the dead peer is: <%s>\n", pszNodeUUID);
}

void PushToConvoyDataCacheReplicationController::newLinkToNeighbor (const char *pszNodeUID,
                                                                    const char *pszPeerRemoteAddr,
                                                                    const char *pszIncomingInterface)
{   
}

void PushToConvoyDataCacheReplicationController::droppedLinkToNeighbor (const char *pszNodeUID,
                                                                        const char *pszPeerRemoteAddr)
{
}

void PushToConvoyDataCacheReplicationController::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
}

void PushToConvoyDataCacheReplicationController::dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad)
{
    checkAndLogMsg ("PushToConvoyDataCacheReplicationController::dataCacheUpdated",
                    Logger::L_Info, "The data cache has been updated with.\n");
    _i64LastCacheUpdate = getTimeInMilliseconds ();
}

void PushToConvoyDataCacheReplicationController::disServiceControllerMsgArrived (ControllerToControllerMsg *pCtrlMsg)
{
    // TODO" implement this
    checkAndLogMsg ("PushToConvoyDataCacheReplicationController::disServiceControllerMsgArrived",
                    Logger::L_SevereError, "Method not yet implemented.\n");
}

void PushToConvoyDataCacheReplicationController::disServiceControlMsgArrived (DisServiceCtrlMsg *pCtrlMsg)
{
}

void PushToConvoyDataCacheReplicationController::disServiceDataMsgArrived (DisServiceDataMsg *pDataMsg)
{
}
