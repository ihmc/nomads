/*
 * PeerState.cpp
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

#include "PeerState.h"

#include "BasicWorldState.h"
#include "DisServiceDefs.h"
#include "NodeInfo.h"
#include "WorldState.h"
#include "TopologyWorldState.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

PeerState::PeerState (Type type)
{
    _type = type;
    _pLocalNodeInfo = NULL;
}

PeerState::~PeerState()
{
}

PeerState::Type PeerState::getType (void)
{
    return _type;
}

PeerState * PeerState::getInstance (DisseminationService *pDisService, Type type)
{
    switch (type) {
        case NEIGHBOR_STATES:
            //return new BasicWorldState (pDisService);

        case TOPOLOGY_STATE:
            return new WorldState (pDisService);
        
        case IMPROVED_TOPOLOGY_STATE:
            return new TopologyWorldState (pDisService);

        default:
            return NULL;
    }
}

void PeerState::registerWithListeners (DisseminationService *pDisService, PeerState *pPeerState)
{
    unsigned int uiIndex;
    if (pDisService->registerMessageListener (pPeerState, uiIndex) < 0) {
        checkAndLogMsg ("PeerState::registerWithListeners", listerRegistrationFailed,
                        "PeerState", "MessageListener");
        exit (-1);
    }

    switch (pPeerState->getType()) {
        case NEIGHBOR_STATES: {
            // BasicWorldState *pBWS = (BasicWorldState*) pPeerState;
            break;
        }

        case TOPOLOGY_STATE: {
            WorldState *pWS = (WorldState*) pPeerState;
            if (pDisService->registerGroupMembershiListener (pWS, uiIndex) < 0) {
                checkAndLogMsg ("PeerState::registerWithListeners", listerRegistrationFailed,
                                "WorldState", "GroupMembershiListener");
                exit (-1);
            }
            break;
        }
        
        case IMPROVED_TOPOLOGY_STATE: {
            TopologyWorldState *pWS = (TopologyWorldState*) pPeerState;
            if (pDisService->registerGroupMembershiListener (pWS, uiIndex) < 0) {
                checkAndLogMsg ("PeerState::registerWithListeners", listerRegistrationFailed,
                                "TopologyWorldState", "GroupMembershiListener");
                exit (-1);
            }
            break;
        }

        default:
            break;
    }
}

void PeerState::setLocalNodeInfo (LocalNodeInfo *pLocalNodeInfo)
{
    _m.lock (71);
    if (pLocalNodeInfo != NULL) {
        _pLocalNodeInfo = pLocalNodeInfo;
    }
    else {
        checkAndLogMsg ("WorldState::setLocalNodeInfo", Logger::L_SevereError,
                        "LocalNodeInfo NULL\n");
    }
    _m.unlock (71);
}

void PeerState::setDeadPeerInterval (uint32 ui32DeadPeerInterval)
{
    _ui32DeadPeerInterval = ui32DeadPeerInterval;
}

int PeerState::deregisterAllPeerStateListeners()
{
    return _notifier.deregisterAllListeners();
}

int PeerState::deregisterPeerStateListener (unsigned int uiIndex)
{
    return _notifier.deregisterListener (uiIndex);
}

int PeerState::registerPeerStateListener (PeerStateListener *pListener)
{
    return _notifier.registerListener (pListener);
}

int PeerState::setIPAddress (const char *pszSenderNodeId, const char *pszIPAddress)
{
    _m.lock (72);
    int rc = -1;
    RemoteNodeInfo *pRNI = getPeerNodeInfo (pszSenderNodeId);
    if (pRNI != NULL) {
        pRNI->setIPAddress (pszIPAddress);
        rc = 0;
    }
    else {
        checkAndLogMsg ("PeerState::setIPAddress", Logger::L_Warning,
                        "Peer %s not found\n", pszSenderNodeId);
    }
    _m.unlock (72);
    return rc;
}

int PeerState::setIPAddress (const char *pszSenderNodeId, uint32 ui32IPAddress)
{
    _m.lock (73);
    char *pszIPAddress = new char[16];
    itoa (pszIPAddress, ui32IPAddress);
    pszIPAddress[15] = '\0';
    int ret = setIPAddress (pszSenderNodeId, pszIPAddress);
    _m.unlock (73);
    return ret;
}

uint32 PeerState::getIPAddress (const char *pszSenderNodeId)
{
    _m.lock (74);
    uint32 ui32IpAddress = 0;
    RemoteNodeInfo *pRNI = getPeerNodeInfo (pszSenderNodeId);
    if (pRNI != NULL) {
        ui32IpAddress = pRNI->getDefaultIPAddress();
    }
    else {
        checkAndLogMsg ("PeerState::getIPAddress", Logger::L_Warning,
                        "Peer %s not found\n", pszSenderNodeId);
    }
    _m.unlock (74);
    return ui32IpAddress;
}

const char * PeerState::getIPAddressAsString (const char *pszSenderNodeId)
{
    _m.lock (75);
    const char *pszIPAddress = NULL;
    RemoteNodeInfo *pRNI = getPeerNodeInfo (pszSenderNodeId);
    if (pRNI != NULL) {
        pszIPAddress = pRNI->getDefaultIPAddressAsString();
    }
    else {
        checkAndLogMsg ("PeerState::getIPAddressAsString", Logger::L_Warning,
                        "Peer %s not found\n", pszSenderNodeId);
    }
    _m.unlock (75);
    return pszIPAddress;
}

const char ** PeerState::getIPAddressesAsStrings (const char *pszSenderNodeId)
{
    _m.lock (76);
    const char **ppszIPAddresses = NULL;
    RemoteNodeInfo *pRNI = getPeerNodeInfo (pszSenderNodeId);
    if (pRNI != NULL) {
        ppszIPAddresses = pRNI->getIPAddressesAsStrings();
    }
    else {
        checkAndLogMsg ("PeerState::getIPAddressesAsStrings", Logger::L_Warning,
                        "Peer %s not found\n", pszSenderNodeId);
    }
    _m.unlock (76);
    return ppszIPAddresses;
}

