/*
 * BasicWorldState.cpp
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

#include "BasicWorldState.h"

#include "DisseminationService.h"
#include "DisServiceMsg.h"
#include "MessageInfo.h"
#include "NodeInfo.h"

#include "ConfigFileReader.h"
#include "BufferWriter.h"
#include "NetUtils.h"
#include "NLFLib.h"
#include "Writer.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

BasicWorldState::BasicWorldState (DisseminationService *pDisService)
    : PeerState (PeerState::NEIGHBOR_STATES),
      _remoteNodes (true, true, true, true),
      _neighborStates(true, true, true, true)
{
    _pDisService = pDisService;

    ConfigFileReader cfgReader (pDisService->_pCfgMgr);
    _bDataCacheStateSeqId = cfgReader.isDataCacheStateEnabled();
    _bTopologyStateSeqId = cfgReader.isTopologyStateEnabled();
    _bSubscriptionStateSeqId = cfgReader.isSubscriptionStateEnabled();
    _ui32DataCacheStateSeqId = _ui32TopologyStateSeqId = _ui32SubscriptionStateSeqId = 1;

    _ui32DeadPeerInterval = DisseminationService::DEFAULT_DEAD_PEER_INTERVAL;
}

BasicWorldState::~BasicWorldState()
{
}

void BasicWorldState::setDeadPeerInterval (uint32 ui32DeadPeerInterval)
{
    _ui32DeadPeerInterval = ui32DeadPeerInterval;
}

int BasicWorldState::incrementDataCacheStateSeqId()
{
    _ui32DataCacheStateSeqId++;
    return 0;
}

int BasicWorldState::incrementTopologyStateSeqId()
{
    _ui32TopologyStateSeqId++;
    return 0;
}

int BasicWorldState::incrementSubscriptionStateSeqId()
{
    _ui32SubscriptionStateSeqId++;
    return 0;
}

void BasicWorldState::newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                          DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress)
{
    _m.lock();
    const char *pszNeighborNodeId = pDisServiceMsg->getSenderNodeId();

    bool bNewPeer = false;

    NeighborState *pNS = _neighborStates.get (pszNeighborNodeId);
    if (pNS == NULL) {
        pNS =  new NeighborState();
        _neighborStates.put (pszNeighborNodeId, pNS);
        bNewPeer = true;
    }
    pNS->i64MostRecentMessageRcvdTime = getTimeInMilliseconds();
    if (pNS->pRNI == NULL) {
        pNS->pRNI = new RemoteNodeInfo (pszNeighborNodeId);
        bNewPeer = true;
    }
    pNS->pRNI->setIPAddress (ui32SourceIPAddress);

    if (bNewPeer) {
        char *pszPeerRemoteAddr = NetUtils::ui32Inetoa (ui32SourceIPAddress);
        char *pszIncomingInterface = NULL;
        _notifier.newNeighbor (pszNeighborNodeId, pszPeerRemoteAddr, pszIncomingInterface);
        free (pszPeerRemoteAddr);
    }

    _m.unlock();
}

uint32 BasicWorldState::getIPAddress (const char *pszSenderNodeId)
{
    RemoteNodeInfo *pRNI = _remoteNodes.get(pszSenderNodeId);
    if (pRNI != NULL) {
        return pRNI->getDefaultIPAddress();
    }
    return 0;
}

const char * BasicWorldState::getIPAddressAsString (const char *pszSenderNodeId)
{
    RemoteNodeInfo *pRNI = _remoteNodes.get (pszSenderNodeId);
    if (pRNI != NULL) {
        return pRNI->getDefaultIPAddressAsString();
    }
    return NULL;
}

void BasicWorldState::setIPAddress (const char *pszSenderNodeId, const char *pszIPAddress)
{
    NeighborState *pNS = _neighborStates.get (pszSenderNodeId);
    if (pNS != NULL) {
        RemoteNodeInfo *pRNI = pNS->pRNI;
        if (pRNI) {
            pRNI->setIPAddress (pszIPAddress);
        }
    }
    else {
        RemoteNodeInfo *pRNI = _remoteNodes.get (pszSenderNodeId);
        if (pRNI != NULL) {
            pRNI->setIPAddress (pszIPAddress);
        }
    }
}

void BasicWorldState::setIPAddress (const char *pszSenderNodeId, uint32 ui32IPAddress)
{
    char * pszIPAddress = new char[16];
    pszIPAddress[15] = '\0';
    itoa (pszIPAddress, ui32IPAddress);
    setIPAddress (pszSenderNodeId, pszIPAddress);
}

int BasicWorldState::sendKeepAliveMsg (uint8 ui8RepCtrlType, uint8 ui8FwdCtrlType)
{   
    _m.lock();
    int iRet = 0;
    DisServiceWorldStateSeqIdMsg *pWSSIMsg = new DisServiceWorldStateSeqIdMsg (_pDisService->getNodeId(), _ui32TopologyStateSeqId,
                                                                               _ui32SubscriptionStateSeqId, _ui32DataCacheStateSeqId,
                                                                               ui8RepCtrlType, ui8FwdCtrlType);

    if (_pLocalNodeInfo != NULL) {
        pWSSIMsg->setNumberOfActiveNeighbors (getNumberOfActiveNeighbors());
        pWSSIMsg->setMemorySpace (_pLocalNodeInfo->getMemorySpace());
        pWSSIMsg->setBandwidth (_pLocalNodeInfo->getBandwidth());
        pWSSIMsg->setNodesInConnectivityHistory (_pLocalNodeInfo->getConnectivityHistoryNodesCount());
        pWSSIMsg->setNodesRepetitivity (0);
    }
    iRet = _pDisService->broadcastDisServiceCntrlMsg (pWSSIMsg, NULL, "Sending Keep Alive Msg");
    delete pWSSIMsg;
    pWSSIMsg = NULL;

    _m.unlock();
    return iRet;
}

int BasicWorldState::updateNeighbors (void)
{
    DArray2<String> deadPeers;
    unsigned int index = 0;
    for (StringHashtable<NeighborState>::Iterator i = _neighborStates.getAllElements(); !i.end(); i.nextElement()) {
        if (!i.getValue()->isStillAlive (_ui32DeadPeerInterval)) {
            deadPeers[index] = i.getKey();    // String makes a copy of i.getKey()
            index++;
            NeighborState * pNS = _neighborStates.remove (i.getKey());
            delete pNS;
            pNS = NULL;
        }
    }

    for (unsigned int i = 0; i < index; i++) {
        _notifier.deadNeighbor ((const char *) deadPeers[i]);
    }

    return 0;
}

bool BasicWorldState::wasNeighbor (const char *pszNewNeighborNodeId)
{
    if (_remoteNodes.get (pszNewNeighborNodeId)) {
        return true;
    }
    return false;
}

bool BasicWorldState::isActiveNeighbor (const char *pszNewNeighborNodeId)
{
    NeighborState *pNS = _neighborStates.get (pszNewNeighborNodeId);
    if (pNS != NULL) {
        return (pNS->isStillAlive (_ui32DeadPeerInterval));
    }
    return false;
}

uint16 BasicWorldState::getNumberOfActiveNeighbors (void)
{
    return _neighborStates.getCount();
}

bool BasicWorldState::isNeighborUpToDate (const char *pszNeighborNodeId, uint32 ui32CurrentWorldStateSeqId)
{
    NeighborState *pNS = _neighborStates.get (pszNeighborNodeId);
    if (pNS != NULL) {
        if (pNS->ui32ExpectedWorldStateSeqId >= ui32CurrentWorldStateSeqId) {
            return true;
        }
    }
    return false;
}

//==============================================================================
// NeighborState
//==============================================================================

BasicWorldState::NeighborState::NeighborState (void)
{
    ui32ExpectedWorldStateSeqId = 0;
    i64MostRecentMessageRcvdTime = getTimeInMilliseconds();
    pRNI = NULL;
}

BasicWorldState::NeighborState::NeighborState (uint32 ui32WorldStateSeqId)
{
    ui32ExpectedWorldStateSeqId = ui32WorldStateSeqId + 1;
    i64MostRecentMessageRcvdTime = getTimeInMilliseconds();
    pRNI = NULL;
}

BasicWorldState::NeighborState::~NeighborState (void)
{
    pRNI = NULL;
}

bool BasicWorldState::NeighborState::isUpToDate (uint32 ui32LastWorldStateSeqIdReceived)
{
    return (ui32LastWorldStateSeqIdReceived < ui32ExpectedWorldStateSeqId);
}

bool BasicWorldState::NeighborState::isStillAlive (uint32 ui32DeadPeerInterval)
{
    return ((getTimeInMilliseconds() - i64MostRecentMessageRcvdTime) <= ui32DeadPeerInterval);
}
