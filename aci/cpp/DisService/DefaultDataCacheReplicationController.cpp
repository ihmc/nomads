/*
 * DefaultDataCacheReplicationController.cpp
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

#include "DefaultDataCacheReplicationController.h"

#include "DisseminationService.h"
#include "DisServiceDefs.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

DefaultDataCacheReplicationController::DefaultDataCacheReplicationController (DisseminationService *pDisService, bool bRequireAck)
    : DataCacheReplicationController (DCRC_Default, pDisService, bRequireAck)
{
}

DefaultDataCacheReplicationController::DefaultDataCacheReplicationController (DisseminationServiceProxyServer *pDisServiceProxy)
        : DataCacheReplicationController (DCRC_Default, pDisServiceProxy)
{
}

DefaultDataCacheReplicationController::~DefaultDataCacheReplicationController (void)
{
}

void DefaultDataCacheReplicationController::newNeighbor (const char *pszNodeUUID,
                                                         const char *pszPeerRemoteAddr,
                                                         const char *pszIncomingInterface)
{
    checkAndLogMsg ("DefaultDataCacheReplicationController::newNeighbor", Logger::L_Info,
                    "the new peer is: <%s>\n", pszNodeUUID);
}

void DefaultDataCacheReplicationController::deadNeighbor (const char *pszNodeUUID)
{
    checkAndLogMsg ("DefaultDataCacheReplicationController::deadNeighbor", Logger::L_Info,
                    "the dead peer is: <%s>\n", pszNodeUUID);
}

void DefaultDataCacheReplicationController::newLinkToNeighbor (const char *pszNodeUID,
                                                               const char *pszPeerRemoteAddr,
                                                               const char *pszIncomingInterface)
{
}

void DefaultDataCacheReplicationController::droppedLinkToNeighbor (const char *pszNodeUID,
                                                                   const char *pszPeerRemoteAddr)
{
}

void DefaultDataCacheReplicationController::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
    checkAndLogMsg ("DefaultDataCacheReplicationController::stateUpdateForPeer", Logger::L_Info,
                    "the updated peer is: <%s>\n", pszNodeUID);
}

void DefaultDataCacheReplicationController::dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad)
{
    checkAndLogMsg ("DefaultDataCacheReplicationController::dataCacheUpdated", Logger::L_Info,
                    "the data cache has been updated with.\n");
}

void DefaultDataCacheReplicationController::disServiceControllerMsgArrived (ControllerToControllerMsg *pCtrlMsg)
{
}

void DefaultDataCacheReplicationController::disServiceControlMsgArrived (DisServiceCtrlMsg *pCtrlMsg)
{
}

void DefaultDataCacheReplicationController::disServiceDataMsgArrived (DisServiceDataMsg *pDataMsg)
{
}

