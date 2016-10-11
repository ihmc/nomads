/*
 * TopologyWorldState.cpp
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

#include "TopologyWorldState.h"

#include "DisServiceDefs.h"
#include "DisServiceMsg.h"
#include "Subscription.h"
#include "MessageInfo.h"

#include "BufferWriter.h"
#include "DArray.h"
#include "InetAddr.h"
#include "Logger.h"
#include "MSPAlgorithm.h"
#include "NetUtils.h"
#include "NLFLib.h"
#include "StrClass.h"
#include "Writer.h"

#include "ConfigFileReader.h"
#include "SubscriptionList.h"
#include "Message.h"

#include "StringTokenizer.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

TopologyWorldState::TopologyWorldState (DisseminationService *pDisService)
    : WorldState (pDisService)
{
    _type = PeerState::IMPROVED_TOPOLOGY_STATE;

    _bSubscriptionsExchangeEnabled = false;
    _bTopologyExchangeEnabled = false;

    _pISSMsg = NULL; // Improved subscription state message
    _bSendReq = false;

    _iLastAging = 0;
}

TopologyWorldState::~TopologyWorldState (void)
{
    if (_pISSMsg) {
        delete (_pISSMsg->getSubscriptionsTable());
        delete (_pISSMsg->getNodesTable());
        delete _pISSMsg;
    }
}

//==============================================================================
//  UTILITIES MODULE
//==============================================================================

void TopologyWorldState::setSubscriptionsExchangeEnabled (bool bSubscriptionsExchangeEnabled)
{
    _bSubscriptionsExchangeEnabled = bSubscriptionsExchangeEnabled;
}

void TopologyWorldState::setTopologyExchangeEnabled (bool bTopologyExchangeEnabled)
{
    _bTopologyExchangeEnabled = bTopologyExchangeEnabled;
}

void TopologyWorldState::setParameters (float fProbContact, float fProbThreshold,
                                        float fAddParam, float fAgeParam) {

    _fProbContact = fProbContact;
    _fProbThreshold = fProbThreshold;
    _fAddParam = fAddParam;
    _fAgeParam = fAgeParam;
}

void TopologyWorldState::newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                             DisServiceMsg *pDisServiceMsg, uint32 ui32IPAddress,
                                             const char *pszIncomingInterface)
{
    _mPeerStateNotification.lock();
    _m.lock (100);
    Event *pEvent = NULL;
    switch (pDisServiceMsg->getType()) {

        case DisServiceMsg::DSMT_Data:
        case DisServiceMsg::DSMT_DataReq:
        {
            addOrActivateNeighbor (pDisServiceMsg->getSenderNodeId(), ui32IPAddress, pEvent);
            break;
        }
        
        case DisServiceMsg::DSMT_WorldStateSeqId:
        {
            DisServiceWorldStateSeqIdMsg *pDSWSMsg = (DisServiceWorldStateSeqIdMsg *) pDisServiceMsg;
            addOrActivateNeighbor (pDisServiceMsg->getSenderNodeId(), ui32IPAddress, pDSWSMsg, pEvent);
            if (_bSubscriptionsExchangeEnabled) {
                bool iSSUTD = isSubscriptionStateUpToDate (pDisServiceMsg->getSenderNodeId(), pDSWSMsg->getSubscriptionStateCRC());
                if (!iSSUTD || (pDSWSMsg->getSubscriptionStateCRC() != _ui16SubscriptionStateCRC)) { 
                    // SubscriptionState of the node is not uptodate OR CRCreceived != myCRC
                    DisServiceSubscriptionStateReqMsg *pSSReqMsg = new DisServiceSubscriptionStateReqMsg();
                    pSSReqMsg->setSenderNodeId (_pDisService->getNodeId());
                    pSSReqMsg->setTargetNodeId (pDisServiceMsg->getSenderNodeId());
                    _bSendReq = true;
                    sendSubscriptionStateReqMsg (pSSReqMsg);
                }
            }
            break;
        }

        case DisServiceMsg::DSMT_ImprovedSubStateMessage:
        {
            addOrActivateNeighbor (pDisServiceMsg->getSenderNodeId(), ui32IPAddress, pEvent);
            if (_bSubscriptionsExchangeEnabled) {
                DisServiceImprovedSubscriptionStateMsg *pDSISSMsg = (DisServiceImprovedSubscriptionStateMsg *) pDisServiceMsg;
                updateSubscriptionState (pDSISSMsg->getSubscriptionsTable(), pDSISSMsg->getNodesTable());
            }
            break;
        }

        case DisServiceMsg::DSMT_SubStateReq:
        {
            addOrActivateNeighbor (pDisServiceMsg->getSenderNodeId(), ui32IPAddress, pEvent);
            if (_bSubscriptionsExchangeEnabled) {
                DisServiceSubscriptionStateReqMsg *pDSSRMsg = (DisServiceSubscriptionStateReqMsg *) pDisServiceMsg;
                receivedSubscriptionStateReqMsg (pDSSRMsg);
                if ((pDSSRMsg->getTargetNodeId() != NULL) && (strNotNullAndEqual (_pDisService->getNodeId(), pDSSRMsg->getTargetNodeId()))) {
                    sendSubscriptionStateMsg (pDSSRMsg->getSenderNodeId());
                } else if (pDSSRMsg->getTargetNodeId() == NULL) {
                    float fProbability = getStatefulForwardProbability (pDSSRMsg->getSenderNodeId());
                    if (((rand() % 100) + 1.0f) < fProbability) {
                        sendSubscriptionStateMsg (pDSSRMsg->getSenderNodeId());
                    }
                }
            }
            break;
        }
        
        case DisServiceMsg::DSMT_ProbabilitiesMsg:
        {
            addOrActivateNeighbor (pDisServiceMsg->getSenderNodeId(), ui32IPAddress, pEvent);
            if (_bSubscriptionsExchangeEnabled) {
                DisServiceProbabilitiesMsg *pDSPMsg = (DisServiceProbabilitiesMsg *) pDisServiceMsg;
                updateIndirectProbabilities(pDSPMsg->getSenderNodeId(), pDSPMsg->getProbabilitiesTable());
            }
            break;
        }

        default:
            addOrActivateNeighbor (pDisServiceMsg->getSenderNodeId(), ui32IPAddress, pEvent);
            break;
    }

    _m.unlock (100);
    
    if (pEvent != NULL) {
        switch (pEvent->type) {
            case NewNeighbor:
            {
                char *pszPeerRemoteAddr = NetUtils::ui32Inetoa (ui32IPAddress);
                _notifier.newNeighbor (pEvent->pszNodeId, pszPeerRemoteAddr, pszIncomingInterface);
                _ui32TotalNumberOfPeer++;
                checkAndLogMsg ("TopologyWorldState::newIncomingMessage", Logger::L_Info, "notifying newNeighbor %s\n", pEvent->pszNodeId);
                free (pszPeerRemoteAddr);
                break;
            }

            case DeadNeighbor:
            {
                _notifier.deadNeighbor (pEvent->pszNodeId);
                if (_ui32TotalNumberOfPeer > 0) {
                    _ui32TotalNumberOfPeer--;
                }
                checkAndLogMsg ("TopologyWorldState::newIncomingMessage", Logger::L_Info, "notifying deadNeighbor %s\n", pEvent->pszNodeId);
                break;
            }
            case NewLink:
            {
                _notifier.newLinkToNeighbor (pEvent->pszNodeId, ((NewLinkEvent *) pEvent)->pszNewLinkAddr,
                                             pszIncomingInterface);
                checkAndLogMsg ("TopologyWorldState::newIncomingMessage", Logger::L_Info, "notifying newLink to newNeighbor %s\n", pEvent->pszNodeId);
                break;
            }
            case Update:
            {
                _notifier.stateUpdateForPeer (pEvent->pszNodeId, ((UpdateEvent*) pEvent)->pUpdate);
                checkAndLogMsg ("TopologyWorldState::newIncomingMessage", Logger::L_Info, "notifying stateUpdateForPeer for peer %s\n", pEvent->pszNodeId);
                break;
            }
        }
        delete pEvent;
    }
    _mPeerStateNotification.unlock();
}

//==============================================================================
//  SUBSCRIPTION MODULE
//==============================================================================

void TopologyWorldState::incrementSubscriptionStateSeqId (void)
{
    // Updates the local subscription info and then calls sendSubscriptionStateMsg
    _m.lock (108);
    if (_pLocalNodeInfo == NULL) {
        _m.unlock (108);
        return;
    }
    uint32 ui32SubscriptionStateSeqId = _pLocalNodeInfo->getSubscriptionStateSequenceID();
    uint32 *pui32SeqId = _subscriptionStateTable.get (_pDisService->getNodeId());
    if (pui32SeqId == NULL) {
        pui32SeqId = new uint32[1];
        if (pui32SeqId == NULL) {
            checkAndLogMsg ("TopologyWorldState::incrementSubscriptionStateSeqId", memoryExhausted);
            _m.unlock (108);
            return;
        }
        _subscriptionStateTable.put (_pDisService->getNodeId(), pui32SeqId);
    }
    pui32SeqId[0] = ui32SubscriptionStateSeqId;
    _crc.reset();
    for (StringHashtable<uint32>::Iterator iterator = _subscriptionStateTable.getAllElements(); !iterator.end(); iterator.nextElement()) {
        _crc.update ((const char *) iterator.getKey());
        _crc.update32 (iterator.getValue());
    }
    _ui16SubscriptionStateCRC = _crc.getChecksum();
    // The history table is changed so reset the sub state req and update the
    // sub state msg if there is any or send the new subscription
    delete _pSSReqMsg;
    _pSSReqMsg = NULL;
    SubscriptionList *pSubscriptions = _pLocalNodeInfo->getConsolidatedSubscriptionsCopy();
    sendSubscriptionStateMsg (pSubscriptions, _pDisService->getNodeId(), &ui32SubscriptionStateSeqId);
    _m.unlock (108);
}

int TopologyWorldState::sendSubscriptionStateMsg (const char *pszNodeId) 
{
    // Creates a subscription msg for pszNodeId with the missing subscriptions
    _m.lock (125);
    // pSubscriptionsTable: nodeId->subscriptionList; subscriptionList: groupName->subscription
    // pNodesTable: nodeId->sequenceId
    StringHashtable<SubscriptionList> *pSubscriptionsTable = NULL;
    StringHashtable<uint32> *pNodesTable = NULL;
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*) _pLocalNodeInfo->get (pszNodeId);
    if (pRNI != NULL) {
        if (_pISSMsg) {
            pSubscriptionsTable = _pISSMsg->getSubscriptionsTable();
            pNodesTable = _pISSMsg->getNodesTable();
        } else {
            pSubscriptionsTable = new StringHashtable<SubscriptionList> (true, true, true, true);
            pNodesTable = new StringHashtable<uint32> (true, true, true, true);
        }
        StringHashtable<uint32> *pRemoteSubscriptionStateHistory = pRNI->getRemoteSubscriptionStateTable ();
        for (StringHashtable<uint32>::Iterator iterator = _subscriptionStateTable.getAllElements(); !iterator.end(); iterator.nextElement()) {
            uint32 *pui32RemoteSubscriptionSeqId = NULL;
            if (pRemoteSubscriptionStateHistory) {
                pui32RemoteSubscriptionSeqId = pRemoteSubscriptionStateHistory->get (iterator.getKey());
            }
            if (!pRemoteSubscriptionStateHistory || !pui32RemoteSubscriptionSeqId || ((*pui32RemoteSubscriptionSeqId) < *(iterator.getValue()))) {
                String nodeId = iterator.getKey();
                NodeInfo * pNI = retrieveNodeInfo (nodeId);
                if (pNI) {
                    if (0 == stricmp(nodeId, _pDisService->getNodeId()) ) { // Local node
                        LocalNodeInfo* pLNI = (LocalNodeInfo*) pNI;
                        pSubscriptionsTable->put (pLNI->getNodeId(), pLNI->getConsolidatedSubscriptionsCopy());
                    } else { // Remote Node
                        RemoteNodeInfo* pRNI = (RemoteNodeInfo*) pNI;
                        pSubscriptionsTable->put (pRNI->getNodeId(), pRNI->getRemoteSubscriptionsCopy());
                    }
                    uint32 *pui32SeqId = pNodesTable->get (nodeId);
                    if (!pui32SeqId) {
                        pui32SeqId = new uint32();
                    }
                    (*pui32SeqId) = *(_subscriptionStateTable.get (nodeId));
                    pNodesTable->put (nodeId, pui32SeqId);
                }
            }
        }
    } else {
        return -1;
    }
    if (pNodesTable->getCount() > 0) {
        _pISSMsg = new DisServiceImprovedSubscriptionStateMsg (_pDisService->getNodeId(), pSubscriptionsTable, pNodesTable);
    }
    _m.unlock (125);
    return 0;
}

int TopologyWorldState::sendSubscriptionStateMsg (SubscriptionList *pSubscriptions, const char *pszNodeId, uint32 *pui32UpdatedSeqId)
{
    // Updates subscription msg with pSubscriptions info
    _m.lock (126);
    bool bAreAllNeighborsUpdated = true;
    // If my neighbors are already updated don't send the subscription state
    for (StringHashtable<Thing>::Iterator iNeighbors = _pLocalNodeInfo->iterator(); !iNeighbors.end() && bAreAllNeighborsUpdated; iNeighbors.nextElement()) {
        if (0 == stricmp (iNeighbors.getKey(), pszNodeId)) {
            continue;
        }
        RemoteNodeInfo *pRNI = (RemoteNodeInfo*) iNeighbors.getValue();
        if (pRNI) {
            if (pRNI->getExpectedSubscriptionStateCRC() != _ui16SubscriptionStateCRC) {
                bAreAllNeighborsUpdated = false;
            }
        }
    }
    if (!bAreAllNeighborsUpdated || _bSendReq) {
        StringHashtable<SubscriptionList> *pSubscriptionsTable = NULL;
        StringHashtable<uint32> *pNodesTable = NULL;
        if (_pISSMsg) {
            pSubscriptionsTable = _pISSMsg->getSubscriptionsTable();
            pNodesTable = _pISSMsg->getNodesTable();
        } else {
            pSubscriptionsTable = new StringHashtable<SubscriptionList> (true, true, true, true);
            pNodesTable = new StringHashtable<uint32> (true, true, true, true);
            _pISSMsg = new DisServiceImprovedSubscriptionStateMsg (_pDisService->getNodeId(), pSubscriptionsTable, pNodesTable);
        }
        uint32 *pui32SeqId = pNodesTable->get (pszNodeId);
        if ((!pui32SeqId) || (*pui32SeqId) != (*pui32UpdatedSeqId)) {
            uint32 *pui32OldSeqId = pNodesTable->remove (pszNodeId);
            delete pui32OldSeqId;
            pui32OldSeqId = NULL;
            uint32 *pui32NewSeqId = new uint32();
            (*pui32NewSeqId) = (*pui32UpdatedSeqId);
            pNodesTable->put (pszNodeId, pui32NewSeqId);
            pSubscriptionsTable->put (pszNodeId, pSubscriptions);
        }
    }
    _m.unlock (126);
    return 0;
}

int TopologyWorldState::sendSubscriptionState (void)
{
    // Sends, if necessary, the subscription msg or the subscription request msg
    _m.lock (127);
    int iRet = 0;
    bool bAreAllNeighborsUpdated = true;
    if (_pISSMsg || _pSSReqMsg) {
        // If my neighbors are already updated don't send the subscription state
        for (StringHashtable<Thing>::Iterator iNeighbors = _pLocalNodeInfo->iterator(); !iNeighbors.end() && bAreAllNeighborsUpdated; iNeighbors.nextElement()) {
            RemoteNodeInfo *pRNI = (RemoteNodeInfo*) iNeighbors.getValue();
            if (pRNI != NULL) {
                if (pRNI->getExpectedSubscriptionStateCRC() != _ui16SubscriptionStateCRC) {
                    if (_pISSMsg) {
                        StringHashtable<uint32> *pNodesTable = _pISSMsg->getNodesTable();
                        // Don't send the subscriptions state if is to update the node that send the last update
                        if ((pNodesTable->getCount() != 1) || (NULL == pNodesTable->get (iNeighbors.getKey()))) {
                            bAreAllNeighborsUpdated = false;
                        }
                    } else {
                        bAreAllNeighborsUpdated = false;
                    }
                }
            }
        }
        if (!bAreAllNeighborsUpdated) {
            if (_pISSMsg) {
                iRet = _pDisService->broadcastDisServiceCntrlMsg (_pISSMsg, NULL, "Sending Subscription State Message");
            } else {
                iRet = _pDisService->broadcastDisServiceCntrlMsg (_pSSReqMsg, NULL, "Sending SubscriptionStateReq Message");
                if (_bSendReq == true) 
                   _bSendReq = false;
            }
        } else { 
            if (_pSSReqMsg && _bSendReq == true) {
                iRet = _pDisService->broadcastDisServiceCntrlMsg (_pSSReqMsg, NULL, "Sending SubscriptionStateReq Message");
                _bSendReq = false;
            }
        }
        if (_pISSMsg) {
            delete (_pISSMsg->getSubscriptionsTable());
            delete (_pISSMsg->getNodesTable());
            delete _pISSMsg;
            _pISSMsg = NULL;
        } else {
            delete _pSSReqMsg;
            _pSSReqMsg = NULL;
        }
    }
    _m.unlock (127);
    return iRet;
}

int TopologyWorldState::sendSubscriptionStateReqMsg (DisServiceSubscriptionStateReqMsg *pSSReqMsg)
{
    // Updates the subscription request msg
    _m.lock (128);
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*) _pLocalNodeInfo->get (pSSReqMsg->getTargetNodeId());
    if (pRNI) {
        if (!_pSSReqMsg) {
            _pSSReqMsg = pSSReqMsg;
            _pSSReqMsg->setSubscriptionStateTable (&_subscriptionStateTable);
        } else if ((_pSSReqMsg->getTargetNodeId() != NULL) && (0 != stricmp (pSSReqMsg->getTargetNodeId(), _pSSReqMsg->getTargetNodeId()))) {
            //TODO: multiple targets
            _pSSReqMsg->setTargetNodeId (NULL);
        }
    }
    _m.unlock (128);
    return 0;
}

int TopologyWorldState::updateSubscriptionState (StringHashtable<SubscriptionList> *pSubscriptionsTable, StringHashtable<uint32> *pNodesTable)
{
    // Updates the remote subscriptions with the pSubscriptionsTable and pNodesTable info
    _m.lock (131);
    for (StringHashtable<uint32>::Iterator iterator = pNodesTable->getAllElements(); !iterator.end(); iterator.nextElement()) {
        if (0 != stricmp (_pDisService->getNodeId(), iterator.getKey())) {
            RemoteNodeInfo * pRNI = (RemoteNodeInfo*) retrieveNodeInfo (iterator.getKey());
            if (pRNI) {
                uint32 *pui32RemoteSeqId = iterator.getValue();
                uint32 *pui32LocalSeqId = _subscriptionStateTable.get (iterator.getKey());
                if (!pui32LocalSeqId) {
                    pui32LocalSeqId = new uint32();
                }
                if ((*pui32RemoteSeqId) > (*pui32LocalSeqId)) {
                    pRNI->unsubscribeAll();
                    (*pui32LocalSeqId) = (*pui32RemoteSeqId);
                    _subscriptionStateTable.put (iterator.getKey(), pui32LocalSeqId);
                    continue;
                }
            }
        }
        pNodesTable->remove (iterator.getKey());
    }
    // Update _ui16SubscriptionStateCRC
    _crc.reset();
    for (StringHashtable<uint32>::Iterator iterator = _subscriptionStateTable.getAllElements(); !iterator.end(); iterator.nextElement()) {
        _crc.update ((const char *) iterator.getKey());
        _crc.update32 (iterator.getValue());
    }
    _ui16SubscriptionStateCRC = _crc.getChecksum();
    for (StringHashtable<SubscriptionList>::Iterator subIterator = pSubscriptionsTable->getAllElements(); !subIterator.end(); subIterator.nextElement()) {
        const char * nodeId = subIterator.getKey();
        if (pNodesTable->get (nodeId) != NULL) {
            SubscriptionList *pSubs = subIterator.getValue();
            RemoteNodeInfo * pRNI = (RemoteNodeInfo*) retrieveNodeInfo (nodeId);
            if (pRNI == NULL) {
                _m.unlock (131);
                return 0;
            }
            // Add subscriptions to pRNI
            for (StringHashtable<Subscription>::Iterator i = pSubs->getIterator(); !i.end(); i.nextElement()) {
                Subscription *pSub = i.getValue();
                Subscription *pSubAux = pSub->clone();
                pRNI->subscribe(i.getKey(), pSubAux);
            }
            SubscriptionList *pSubscriptions =  pRNI->getRemoteSubscriptionsCopy();
            sendSubscriptionStateMsg (pSubscriptions, nodeId, pNodesTable->get (nodeId));
        }
    }
    delete pSubscriptionsTable;
    pSubscriptionsTable = NULL;
    delete pNodesTable;
    pNodesTable = NULL;
    _m.unlock (131);
    return 0;
}

//==============================================================================
//  TOPOLOGY MODULE
//==============================================================================

NodeInfo * TopologyWorldState::retrieveNodeInfo (const char *pszNodeId)
{
    // Retrieves NodeInfo using nodeId
    // If node is not found, the node is added to _deadPeers and then returned
    _m.lock(135);
    bool bIsDead = _deadPeers.containsKey (pszNodeId);
    bool bIsAlive = _pNodesGraph->contains (pszNodeId);
    NodeInfo *pNI;
    if (bIsDead) {
        pNI = (NodeInfo*) _deadPeers.get (pszNodeId);
    } else if (bIsAlive) {
        pNI = (NodeInfo*) _pNodesGraph->get (pszNodeId);
    } else {
        pNI = new RemoteNodeInfo(pszNodeId);
        _deadPeers.put (pszNodeId, (RemoteNodeInfo*) pNI);
        pNI = (NodeInfo*) _deadPeers.get (pszNodeId); 
    }    
    _m.unlock(135);
    return pNI;
}

int TopologyWorldState::getTimeSinceLastAging (void)
{
    // Returns seconds from last aging
    uint64 iNow = (getTimeInMilliseconds() / 1000);
    return (int) (iNow - _iLastAging); // I'm positive that this difference can be returned as int
}

int TopologyWorldState::ageProbabilities (void)
{
    // Ages indirect probabilities for every known node
    _m.lock (138);
    if (_iLastAging) {
        // Alive neighbors
        for (StringHashtable<Thing>::Iterator iNeighbors = _pLocalNodeInfo->getAllElements(); !iNeighbors.end(); iNeighbors.nextElement()) {
            RemoteNodeInfo *pRNI = (RemoteNodeInfo*) iNeighbors.getValue();
            pRNI->ageIndirectProbabilities (_fAgeParam, _fProbThreshold, getTimeSinceLastAging());
        }
        // Dead peers
        for (StringHashtable<RemoteNodeInfo>::Iterator iDead = _deadPeers.getAllElements(); !iDead.end(); iDead.nextElement()) {
            RemoteNodeInfo *pRNI = (RemoteNodeInfo*) iDead.getValue();
            pRNI->ageIndirectProbabilities (_fAgeParam, _fProbThreshold, getTimeSinceLastAging());
        }
        _iLastAging = (getTimeInMilliseconds() / 1000);
    } else {
        _iLastAging = (getTimeInMilliseconds() / 1000);
    }
    _m.unlock (138);
    return 0;
}

StringFloatHashtable * TopologyWorldState::sendProbabilitiesInternal (RemoteNodeInfo *pRNI)
{
    // Create a probabilities table with the info on how to reach remote node pRNI
    StringFloatHashtable *pIndProb = NULL;
    if (isPeerAlive (pRNI)) { // Alive node
        pIndProb = new StringFloatHashtable();
        float fHigh1 = (float) _fProbContact;
        float *pHigh1 = &fHigh1;
        pIndProb->put (_pDisService->getNodeId(), pHigh1);
    }
    else { // Dead node
        if (pRNI->getIndirectProbabilities()) {
            float fHigh1 = 0.0f;
            const char *pszHigh1 = NULL;
            float fHigh2 = 0.0f;
            const char *pszHigh2 = NULL;
            for (StringFloatHashtable::Iterator iNodes = pRNI->getIndirectProbabilities()->getAllElements(); !iNodes.end(); iNodes.nextElement()) {
                if (isActiveNeighbor (iNodes.getKey())) {
                    float prob = *(iNodes.getValue());
                    if (prob > fHigh1 && prob > fHigh2) {
                        fHigh2 = fHigh1;
                        pszHigh2 = pszHigh1;
                        fHigh1 = prob;
                        pszHigh1 = iNodes.getKey();
                    }
                    else if (prob > fHigh2) {
                        fHigh2 = prob;
                        pszHigh2 = iNodes.getKey();
                    }
                }
            }
            if (fHigh1 > 0.0f) { 
                pIndProb = new StringFloatHashtable();
                float *pHigh1 = &fHigh1;
                pIndProb->put (pszHigh1, pHigh1);
                if (fHigh2 > 0.0f) {
                    float *pHigh2 = &fHigh2;
                    pIndProb->put (pszHigh2, pHigh2);
                }
            }
        }
    }
    return pIndProb;
}

int TopologyWorldState::sendProbabilities (void)
{
    // Sends probabilities message if necessary
    _m.lock (139);
    int iRet = 0;
    ageProbabilities();
    // pProbabilitiesTable: nodeId->indirectProbTable; indirectProbTable: gatewayNodeId->probValue
    StringHashtable<StringFloatHashtable> *pProbabilitiesTable = NULL;
    // Alive peers
    for (StringHashtable<Thing>::Iterator iNeighbors = _pLocalNodeInfo->getAllElements(); !iNeighbors.end(); iNeighbors.nextElement()) {
        RemoteNodeInfo *pRNI = (RemoteNodeInfo*) iNeighbors.getValue();
        StringFloatHashtable *pIndProb = sendProbabilitiesInternal (pRNI);
        if (pIndProb) {
            if (pProbabilitiesTable == NULL) {
                pProbabilitiesTable = new StringHashtable<StringFloatHashtable>();
            }
            pProbabilitiesTable->put (pRNI->getNodeId(), pIndProb);
        }
    }
    // Dead peers
    for (StringHashtable<RemoteNodeInfo>::Iterator iDead = _deadPeers.getAllElements(); !iDead.end(); iDead.nextElement()) {
        RemoteNodeInfo *pRNI = (RemoteNodeInfo*) iDead.getValue();
        StringFloatHashtable *pIndProb = sendProbabilitiesInternal (pRNI);
        if (pIndProb) {          
            if (pProbabilitiesTable == NULL) {
                pProbabilitiesTable = new StringHashtable<StringFloatHashtable>();
            }
            pProbabilitiesTable->put (pRNI->getNodeId(), pIndProb);
        }
    }
    if (pProbabilitiesTable) {
        _pPMsg = new DisServiceProbabilitiesMsg (_pDisService->getNodeId(), pProbabilitiesTable);
        iRet = _pDisService->broadcastDisServiceCntrlMsg (_pPMsg, NULL, "Sending Probabilities Message");
    }
    _m.unlock (139);
    return iRet;
}

int TopologyWorldState::updateIndirectProbabilities (const char *pszNeighborNodeId, StringHashtable<StringFloatHashtable> *pProbabilitiesTable)
{
    // Updates indirect probabilities after receiving a probabilities message from pszNeighborNodeId
    _m.lock (140);
    ageProbabilities();
    for (StringHashtable<StringFloatHashtable>::Iterator iterator = pProbabilitiesTable->getAllElements(); !iterator.end(); iterator.nextElement()) {
        if ( 0 != stricmp(iterator.getKey(), _pDisService->getNodeId()) ) { // Info are not about localnode
            RemoteNodeInfo *pRNI = (RemoteNodeInfo *) retrieveNodeInfo (iterator.getKey());
            if (pRNI) {
                // Choose the best indirect probability for node pRNI
                StringFloatHashtable *pIndProb = iterator.getValue();
                float fMaxProb = 0.0f;
                for (StringFloatHashtable::Iterator i = pIndProb->getAllElements(); !i.end(); i.nextElement()) {
                    if ( 0 != stricmp (i.getKey(), _pDisService->getNodeId()) ) { // Localnode is not the gateway node
                        if ( *(i.getValue()) > fMaxProb ) { // Prob is higher than maxProb
                            fMaxProb = *(i.getValue());
                        }
                    }
                }
                if (fMaxProb != 0.0f) {
                    pRNI->addIndirectProbability (pszNeighborNodeId, fMaxProb, _fAddParam, _fProbThreshold);
                }
            }
        }
    }
    _m.unlock (140);
    return 0;
}

int TopologyWorldState::printWorldStateInfo (void) 
{
    // Prints information about local node, like subscriptions, alive neighbors, dead peers, etc
    _m.lock (141);
    checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "=========================================================================\n");
    checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     PRINT WORLD STATE INFO\n");
    checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     Node id: %s\n", _pDisService->getNodeId());
    if ((_pLocalNodeInfo->getConsolidatedSubscriptions())->getCount() == 0) {
        checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     No subscribed groups\n");
    } else {
        _pLocalNodeInfo->printAllSubscribedGroups();
        // Print subscribing clients
        checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     SUBSCRIBING CLIENTS:\n");
        DArray<uint16> *pSubClients = _pLocalNodeInfo->getAllSubscribingClients();
        for (int j = 0; j <= pSubClients->getHighestIndex(); j++) {
            checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     SUB CLIENT N.%d:\n", (*pSubClients)[j]);
            SubscriptionList *pSubsForClient = _pLocalNodeInfo->getSubscriptionListForClient ((*pSubClients)[j]);
            _pLocalNodeInfo->releaseLocalNodeInfo();
            if (pSubsForClient && pSubsForClient->getCount() != 0) {
                for (StringHashtable<Subscription>::Iterator i = pSubsForClient->getIterator(); !i.end(); i.nextElement()) {
                    checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     SUBSCRIPTION:\n");
                    checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     groupname %s\n", i.getKey());
                    Subscription *pS = i.getValue();
                    pS->printInfo();                
                }
            }
        }
        // Print local consolidated subscriptions
        checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     LOCAL CONSOLIDATED SUBSCRIPTIONS:\n");
        SubscriptionList *pSubscriptions = _pLocalNodeInfo->getConsolidatedSubscriptions();
        for (StringHashtable<Subscription>::Iterator i = pSubscriptions->getIterator(); !i.end(); i.nextElement()) {
            checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     SUBSCRIPTION:\n");
            checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     groupname %s\n", i.getKey());
            Subscription *pS = i.getValue();
            pS->printInfo();   
        }
    }
    checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "-------------------------------------------------------------------------\n");
    checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     %d ACTIVE NEIGHBORS\n", _pLocalNodeInfo->getCount());
    for (StringHashtable<Thing>::Iterator iNeighbors = _pLocalNodeInfo->getAllElements(); !iNeighbors.end(); iNeighbors.nextElement()) {
        checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "-------------------------------------------------------------------------\n");
        RemoteNodeInfo *pRNI = (RemoteNodeInfo *) iNeighbors.getValue();
        pRNI->printRemoteNodeInfo();
    }
    checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "-------------------------------------------------------------------------\n");
    checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "     %d DEAD PEERS\n", _deadPeers.getCount());
    for (StringHashtable<RemoteNodeInfo>::Iterator iDead = _deadPeers.getAllElements(); !iDead.end(); iDead.nextElement()) {
        checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "-------------------------------------------------------------------------\n");
        RemoteNodeInfo *pRNI = (RemoteNodeInfo *) iDead.getValue();
        pRNI->printRemoteNodeInfo();
    }
    checkAndLogMsg ("TopologyWorldState::printWorldStateInfo", Logger::L_Info, "=========================================================================\n");
    _m.unlock (141);
    return 0;
}

//==============================================================================
//  FORWARDING MODULE
//==============================================================================

uint8 TopologyWorldState::getForwardingStrategy (DisServiceMsg *pDSMsg)
{
    // Chooses which type of forwarding it's better to use for pDSMsg
    // TODO: before using topology forwarding, it could be a good idea to check if every node has subscription enabled
    // Maybe every node can send this info in the keep alive msg
    // TODO: if I know that there are some interested nodes in this msg,
    // but I don't know how to reach them, it's not a good idea to use topology forwarding
    if (_bTopologyExchangeEnabled && _bSubscriptionsExchangeEnabled 
        && pDSMsg->getType() == DisServiceMsg::DSMT_Data) {
        return ForwardingStrategy::TOPOLOGY_FORWARDING;
    } else {
        return ForwardingStrategy::FLOODING_FORWARDING;
    }
    //return ForwardingStrategy::STATEFUL_FORWARDING;
    //return ForwardingStrategy::FLOODING_FORWARDING;
    //return ForwardingStrategy::PROBABILISTIC_FORWARDING;
}

PtrLList<String> * TopologyWorldState::getInterestedRemoteNodes (DisServiceDataMsg *pDSDMsg)
{
    // Returns a list with the remote nodes interested in pDSDMsg
    _m.lock(143);
    PtrLList<String> *pInterestedRemoteNodes = NULL;
    // Alive nodes
    for (StringHashtable<Thing>::Iterator iNeighbors = _pLocalNodeInfo->getAllElements(); !iNeighbors.end(); iNeighbors.nextElement()) {
        RemoteNodeInfo *pRNI = (RemoteNodeInfo *) iNeighbors.getValue();
        if (pRNI->isNodeInterested (pDSDMsg)) {
            if (pInterestedRemoteNodes == NULL) {
                pInterestedRemoteNodes = new PtrLList<String>();
            }
            pInterestedRemoteNodes->insert (new String (pRNI->getNodeId()));
        }
    }
    // Dead peers
    for (StringHashtable<RemoteNodeInfo>::Iterator iDead = _deadPeers.getAllElements(); !iDead.end(); iDead.nextElement()) {
        RemoteNodeInfo *pRNI = (RemoteNodeInfo *) iDead.getValue();
        if (pRNI->isNodeInterested (pDSDMsg)) {
            if (pInterestedRemoteNodes == NULL) {
                pInterestedRemoteNodes = new PtrLList<String>();
            }
            pInterestedRemoteNodes->insert (new String (pRNI->getNodeId()));
        }
    }
    _m.unlock(143);
    return pInterestedRemoteNodes;
}

const char * TopologyWorldState::getBestGateway (const char *pszTargetNodeId)
{
    // Returns the best gateway node to reach node pszTargetNodeId
    _m.lock (144);
    RemoteNodeInfo * pTRNI = (RemoteNodeInfo*) retrieveNodeInfo (pszTargetNodeId);
    if (pTRNI == NULL) {
        _m.unlock (144);
        return 0;
    }
    float fMaxProb = 0.0;
    const char *pszBestGWNodeID = NULL;
    if (pTRNI->getIndirectProbabilities()) {
        for (StringFloatHashtable::Iterator iGWNodes = pTRNI->getIndirectProbabilities()->getAllElements(); !iGWNodes.end(); iGWNodes.nextElement()) {
            const char *pszGWNodeId = iGWNodes.getKey();
            RemoteNodeInfo *pGWRNI = (RemoteNodeInfo*) retrieveNodeInfo (pszGWNodeId);
            if (pGWRNI) { 
                if (isActiveNeighbor (pszGWNodeId)) {
                    float fProb = *(iGWNodes.getValue()); // Prob from GWnode to targetNode
                    if (fProb > fMaxProb) {
                        fMaxProb = fProb;
                        pszBestGWNodeID = pszGWNodeId;
                    }
                }
            }
        }
    }
    _m.unlock (144);
    return pszBestGWNodeID;
}

PtrLList<String> * TopologyWorldState::getTargetNodes (DisServiceDataMsg *pDSDMsg)
{
    // Returns a list with the target nodes for pDSDMsg
    _m.lock (145);
    PtrLList<String> *pInterestedNodes = getInterestedRemoteNodes (pDSDMsg);
    PtrLList<String> *pTargetNodes = NULL;
    if (pInterestedNodes) {
        for (String *pSubNode = pInterestedNodes->getFirst(); pSubNode; pSubNode = pInterestedNodes->getNext()) {
            bool bActiveTarget = isActiveNeighbor (pSubNode->c_str());
            const char *pszBestGW = NULL;
            if (bActiveTarget) {
                pszBestGW = pSubNode->c_str();
            } else {
                pszBestGW = getBestGateway (pSubNode->c_str());
            }
            if (pszBestGW) {
                if (pTargetNodes == NULL) {
                    pTargetNodes = new PtrLList<String>();
                }
                if (pTargetNodes->search (new String (pszBestGW)) == NULL) {
                    pTargetNodes->insert (new String (pszBestGW));
                }
            }
        }
    }
    _m.unlock (145);
    return pTargetNodes;
}
