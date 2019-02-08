/*
 * WorldState.cpp
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

#include "WorldState.h"

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
#include "DisServiceMsgHelper.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

WorldState::WorldState (DisseminationService *pDisService)
    : PeerState (PeerState::TOPOLOGY_STATE),
      _deadPeers (true,  // bCaseSensitiveKeys
                  true,  // bCloneKeys
                  true,  // bDeleteKeys
                  true), // bDeleteValues
      _subscriptionStateTable (true, // bCaseSensitiveKeys
                               true, // bCloneKeys
                               true, // bDeleteKeys
                               true) // bDeleteValues
{
    _pDisService = pDisService;
    _ui32TopologyStateSeqId = 0;
    _ui32SubscriptionStateSeqId = 0;
    _ui16SubscriptionStateCRC = 0;
    _ui32DataCacheStateSeqId = 0;
    _ui32NumberOfNewPeer = 0;
    _ui32TotalNumberOfPeer = 0;

    _ui32DeadPeerInterval = DisseminationService::DEFAULT_DEAD_PEER_INTERVAL;

    _pLocalNodeInfo = NULL;
    _pNodesGraph = NULL;
    _crc.init();

    _pSSMsg = NULL;
    _pSSReqMsg = NULL;
}

WorldState::~WorldState()
{
    delete _pSSReqMsg;
    _pSSReqMsg = NULL;
    if (_pSSMsg) {
        delete (_pSSMsg->getSubscriptionsTable());
        delete (_pSSMsg->getNodesTable());
        delete _pSSMsg;
    }

    delete (StringHashgraph*)_pNodesGraph;
}

void WorldState::setLocalNodeInfo (LocalNodeInfo *pLocalNodeInfo)
{
    _m.lock (96);
    PeerState::setLocalNodeInfo (pLocalNodeInfo);
    if (_pLocalNodeInfo != NULL) {
        _pNodesGraph = new StringHashgraph();
        _pNodesGraph->put (_pLocalNodeInfo->getId(), _pLocalNodeInfo);
        _pLocalNodeInfo->setGraph (_pNodesGraph);
    }
    else {
        checkAndLogMsg ("WorldState::setLocalNodeInfo", Logger::L_SevereError, "LocalNodeInfo NULL\n");
    }
    _m.unlock (96);
}

DisServiceCtrlMsg * WorldState::getKeepAliveMsg (uint8 ui8RepCtrlType, uint8 ui8FwdCtrlType, uint8 ui8NodeImportance)
{
    _m.lock (97);
    DisServiceWorldStateSeqIdMsg *pWSSIMsg = new DisServiceWorldStateSeqIdMsg (_pDisService->getNodeId(), _ui32TopologyStateSeqId, _ui16SubscriptionStateCRC,
                                                                               _ui32DataCacheStateSeqId, ui8RepCtrlType, ui8FwdCtrlType, ui8NodeImportance);
    _m.unlock (97);

    if (_pLocalNodeInfo) {
        pWSSIMsg->setNumberOfActiveNeighbors (getNumberOfActiveNeighbors());
        pWSSIMsg->setMemorySpace (_pLocalNodeInfo->getMemorySpace());
        pWSSIMsg->setBandwidth (_pLocalNodeInfo->getBandwidth());
        pWSSIMsg->setNodesInConnectivityHistory (_pLocalNodeInfo->getConnectivityHistoryNodesCount());
        pWSSIMsg->setNodesRepetitivity (getRepetitiveness());

        // Publication advertisment
        String group;
        uint32 ui32SeqId = 0;
        if (_pLocalNodeInfo->getGroupPubStateToAdvertise (group, ui32SeqId)) {
            pWSSIMsg->setPubAdv (group, ui32SeqId);
        }

    }
    //iRet = _pDisService->broadcastDisServiceCntrlMsg (pWSSIMsg, "Sending Keep Alive Msg");
    //delete pWSSIMsg;
    //pWSSIMsg = NULL;

    return pWSSIMsg;
}

bool WorldState::wasNeighbor (const char *pszNewNeighborNodeId)
{
    _m.lock (98);
    bool bRet = false;
    if (_pLocalNodeInfo) {
        bRet = _pLocalNodeInfo->contains (pszNewNeighborNodeId);
    }
    _m.unlock (98);
    return bRet;
}

bool WorldState::isActiveNeighbor (const char *pszNewNeighborNodeId)
{
    _m.lock (99);
    bool bRet = false;
    Thing *pNeighbor = _pLocalNodeInfo->getThing (pszNewNeighborNodeId);
    if (pNeighbor) {
        // if there is not an entry for the node or if the node is dead
        // return true else if there is an entry but the node is dead return
        // false
        bRet =  isPeerAlive((RemoteNodeInfo*)pNeighbor);
    }
    _m.unlock (99);
    return bRet;
}

void WorldState::newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
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
            addOrActivateNeighbor (pDSWSMsg->getSenderNodeId(), ui32IPAddress, pDSWSMsg, pEvent);
            break;
        }

        case DisServiceMsg::DSMT_SubStateMessage:
        {
            addOrActivateNeighbor (pDisServiceMsg->getSenderNodeId(), ui32IPAddress, pEvent);
            DisServiceSubscriptionStateMsg *pDSSMsg = (DisServiceSubscriptionStateMsg *) pDisServiceMsg;
            updateSubscriptionState (pDSSMsg->getSubscriptionsTable(), pDSSMsg->getNodesTable());
            break;
        }

        case DisServiceMsg::DSMT_SubStateReq:
        {
            addOrActivateNeighbor (pDisServiceMsg->getSenderNodeId(), ui32IPAddress, pEvent);
            DisServiceSubscriptionStateReqMsg *pDSSRMsg = (DisServiceSubscriptionStateReqMsg*) pDisServiceMsg;
            receivedSubscriptionStateReqMsg (pDSSRMsg);
            bool bTargetSpecified = false;
            bool isTarget = DisServiceMsgHelper::isTarget (_pDisService->getNodeId(), pDSSRMsg, bTargetSpecified);
            if (bTargetSpecified && isTarget) {
                sendSubscriptionStateMsg (pDSSRMsg->getSenderNodeId());
            }
            else if (!bTargetSpecified) {
                float fProbability = getStatefulForwardProbability (pDSSRMsg->getSenderNodeId());
                if (((rand() % 100) + 1.0f) < fProbability) {
                    sendSubscriptionStateMsg (pDSSRMsg->getSenderNodeId());
                }
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
                checkAndLogMsg ("WorldState::newIncomingMessage", Logger::L_Info, "notifying newNeighbor %s\n", pEvent->pszNodeId);
                free (pszPeerRemoteAddr);
                break;
            }

            case DeadNeighbor:
                _notifier.deadNeighbor (pEvent->pszNodeId);
                if (_ui32TotalNumberOfPeer > 0) {
                    _ui32TotalNumberOfPeer--;
                }
                checkAndLogMsg ("WorldState::newIncomingMessage", Logger::L_Info, "notifying deadNeighbor %s\n", pEvent->pszNodeId);
                break;

            case NewLink:
                _notifier.newLinkToNeighbor (pEvent->pszNodeId, ((NewLinkEvent *)pEvent)->pszNewLinkAddr,
                                             pszIncomingInterface);
                checkAndLogMsg ("WorldState::newIncomingMessage", Logger::L_Info, "notifying newLink to newNeighbor %s\n", pEvent->pszNodeId);
                break;

            case Update:
                _notifier.stateUpdateForPeer (pEvent->pszNodeId, ((UpdateEvent*)pEvent)->pUpdate);
                checkAndLogMsg ("WorldState::newIncomingMessage", Logger::L_Info, "notifying stateUpdateForPeer for peer %s\n", pEvent->pszNodeId);
                break;
        }
        delete pEvent;
    }
    _mPeerStateNotification.unlock();
}

int WorldState::addOrActivateNeighbor (const char *pszNeighborNodeId, uint32 ui32IPAddress, Event *&pEvent)
{
    _m.lock (101);
    pEvent = NULL;
    int rc = addOrActivateNeighborInternal (pszNeighborNodeId, ui32IPAddress, NULL, pEvent);
    _m.unlock (101);
    return rc;
}

int WorldState::addOrActivateNeighbor (const char *pszNeighborNodeId, uint32 ui32IPAddress,
                                       DisServiceWorldStateSeqIdMsg *pDSWSMsg,
                                       Event *&pEvent)
{
    _m.lock (102);
    pEvent = NULL;
    int rc = addOrActivateNeighborInternal (pszNeighborNodeId, ui32IPAddress, pDSWSMsg, pEvent);
    _m.unlock (102);
    return rc;
}

int WorldState::addOrActivateNeighborInternal (const char *pszNeighborNodeId, uint32 ui32IPAddress,
                                               DisServiceWorldStateSeqIdMsg *pDSWSMsg, Event *&pEvent)
{
    pEvent = NULL;

    // Look if the node pszNeighborNodeId is already a neighbor
    InetAddr inetAddr (ui32IPAddress);

    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->getThing (pszNeighborNodeId);
    if (pRNI == NULL) {
        // It is not a neighbor, check if it is a remote node (a node at X > 1 hops)
        pRNI = (RemoteNodeInfo*)_pNodesGraph->get (pszNeighborNodeId);
        if (pRNI == NULL) {
            // Look if it is in the dead peer
            pRNI = _deadPeers.remove (pszNeighborNodeId);
            if (pRNI == NULL) {
                // it is not a dead peer, it is a node that had never been met,
                // before. Create a new RemoteNodeInfo and increase the number of
                // distinct peers met.
                // NOTE: RemoteNodeInfo copies pszNeighborNodeId
                if (pDSWSMsg != NULL) {
                    pRNI = new RemoteNodeInfo (pszNeighborNodeId,
                                               pDSWSMsg->getTopologyStateUpdateSeqId(),
                                               pDSWSMsg->getSubscriptionStateCRC(),
                                               pDSWSMsg->getDataCacheStateUpdateSeqId());
                }
                else {
                    pRNI = new RemoteNodeInfo (pszNeighborNodeId);
                }

                if (pRNI) {
                    _ui32NumberOfNewPeer++;
                }
                else {
                    checkAndLogMsg ("WorldState::addOrActivateNeighbor", Logger::L_SevereError,
                                    "Memory for new istance of RemoteNodeInfo could not be allocated\n");
                    return -1;
                }
            }
            else {
                // It is a dead peer, increment the number
                // of times the node has been met
                pRNI->incrementOccurrence();
            }
            _pNodesGraph->put (pszNeighborNodeId, pRNI);
            pRNI->setGraph (_pNodesGraph);
            incrementTopologyStateSeqId();
        }
        _pLocalNodeInfo->put (pszNeighborNodeId, pRNI);

        char *pszNeighborNodeIdCpy = strDup (pszNeighborNodeId);
        if (pszNeighborNodeIdCpy == NULL) {
            checkAndLogMsg ("WorldState::addOrActivateNeighborInternal", memoryExhausted);
        }
        else if ((pEvent = new NewNeighborEvent (pszNeighborNodeIdCpy)) == NULL) {
            checkAndLogMsg ("WorldState::addOrActivateNeighborInternal", memoryExhausted);
        }
    }

    if (pRNI != NULL) {
        if (pDSWSMsg != NULL) {
            pRNI->setNumberOfActiveNeighbors (pDSWSMsg->getNumberOfActiveNeighbors());
            pRNI->setMemorySpace (pDSWSMsg->getMemorySpace());
            pRNI->setBandwidth (pDSWSMsg->getBandwidth());
            pRNI->setNodesInConnectivityHistory (pDSWSMsg->getNodesInConnectivityHistory());
            pRNI->setNodesRepetitivity (pDSWSMsg->getNodesRepetitivity());
            pRNI->setNodeImportance (pDSWSMsg->getNodeImportance());
        }

        if (pRNI->setMostRecentMessageRcvdTime (inetAddr.getIPAsString()) && pEvent == NULL) {
            char *pszNeighborNodeIdCpy = strDup (pszNeighborNodeId);
            char *pszNeighborNewAddr = strDup (inetAddr.getIPAsString());
            if (pszNeighborNodeIdCpy == NULL || pszNeighborNewAddr == NULL) {
                checkAndLogMsg ("WorldState::addOrActivateNeighborInternal", memoryExhausted);
                if (pszNeighborNodeIdCpy != NULL) {
                    free (pszNeighborNodeIdCpy);
                }
                pszNeighborNodeIdCpy = NULL;
                if (pszNeighborNewAddr != NULL) {
                    free (pszNeighborNewAddr);
                }
                pszNeighborNewAddr = NULL;
            }
            else if ((pEvent = new NewLinkEvent (pszNeighborNodeIdCpy, pszNeighborNewAddr)) == NULL) {
                checkAndLogMsg ("WorldState::addOrActivateNeighborInternal", memoryExhausted);
            }
        }

        /*char *pszNeighborNodeIdCpy = strDup (pszNeighborNodeId);
        if (pszNeighborNodeIdCpy == NULL) {
            checkAndLogMsg ("WorldState::addOrActivateNeighborInternal", memoryExhausted);
        }
        else if ((pEvent = new NewNeighborEvent (pszNeighborNodeIdCpy)) == NULL) {
            checkAndLogMsg ("WorldState::addOrActivateNeighborInternal", memoryExhausted);
        }*/
    }

    return 0;
}

struct Links
{
    String peerNodeId;
    DArray2<String> ifaces;
};

int WorldState::updateNeighbors (void)
{
    _mPeerStateNotification.lock();
    _m.lock (103);
    DArray2<String> deadNeighbors;
    DArray2<Links> droppedLinks;
    unsigned short uiDeadNeighborCount = 0;
    for (StringHashtable<Thing>::Iterator iNeighbors = _pLocalNodeInfo->getAllElements(); !iNeighbors.end(); iNeighbors.nextElement()) {
        // For each neighboring node, check if it is still neighboring
        RemoteNodeInfo *pRNI = (RemoteNodeInfo*)iNeighbors.getValue();
        if (pRNI == NULL) {
            continue;
        }
        if (isPeerAlive (pRNI)) {
            int i = droppedLinks.firstFree();
            droppedLinks[i].peerNodeId = iNeighbors.getKey();
            DArray2<String> linksToDrop;
            pRNI->getAndRemoveLinksToDrop (_ui32DeadPeerInterval, linksToDrop);
            for (unsigned int j = 0; j < linksToDrop.size(); j++) {
                if (linksToDrop.used (j)) {
                    droppedLinks[i].ifaces[j] = linksToDrop[j];
                }
            }
        }
        else {
            // Delete the node and its child, call the deadPeer and increment the topology state
            String removedNodeId (iNeighbors.getKey()); // NOTE: _pLocalNodeInfo->remove
                                                        // deletes the key, thus I need to
                                                        // make a copy of it.
            RemoteNodeInfo *pRemovedRNI = (RemoteNodeInfo*)_pLocalNodeInfo->remove (removedNodeId);
            if (pRemovedRNI != NULL) {
                incrementTopologyStateSeqId();
                //moveChildToDeadPeers (pRemovedRNI);
                if (pRemovedRNI->getCount() == 0) {
                    // and move it to the set of the dead peers
                    _pNodesGraph->remove (removedNodeId);
                    _deadPeers.put (pRemovedRNI->getId(), pRemovedRNI); // deadPeers make its own copy
                                                                        // of pRemovedRNI->getId()
                }

                // Notify the listeners that there was a dead neighbor
                deadNeighbors[uiDeadNeighborCount] = pRemovedRNI->getId();  // String makes a copy of the ID
                uiDeadNeighborCount++;

                // Re-fetch the iterator since removing an item caused the iterator to be invalid
                // Change later to remove the item in the code below when looping through the dead neighbors
                iNeighbors = _pLocalNodeInfo->getAllElements();
            }
        }
    }
    _m.unlock (103);

    for (unsigned short i = 0; i < deadNeighbors.size(); i++) {
        if (deadNeighbors.used (i)) {
            checkAndLogMsg ("WorldState::updateNeighbors", Logger::L_Info,
			                "notifying deadNeighbor %s\n", deadNeighbors[i].c_str());
            _notifier.deadNeighbor (deadNeighbors[i]);
        }
    }

    for (unsigned short i = 0; i < droppedLinks.size(); i++) {
        if (droppedLinks.used (i)) {
            for (unsigned short j = 0; j < droppedLinks[i].ifaces.size(); j++) {
                if (droppedLinks[i].ifaces.used (j)) {
                    _notifier.droppedLinkToNeighbor (droppedLinks[i].peerNodeId.c_str(),
                                                     droppedLinks[i].ifaces[j].c_str());
                   checkAndLogMsg ("WorldState::updateNeighbors", Logger::L_Info,
                                   "notifying droppedLinkToNeighbor %s (%s)\n",
                                   droppedLinks[i].peerNodeId.c_str(),
                                   droppedLinks[i].ifaces[j].c_str());
                }
            }
        }
    }
    _mPeerStateNotification.unlock();
    return 0;
}

RemoteNodeInfo * WorldState::getNeighborNodeInfo (const char *pszNodeId)
{
    return (RemoteNodeInfo*)_pLocalNodeInfo->getThing (pszNodeId);
}

RemoteNodeInfo ** WorldState::getAllNeighborNodeInfos (void)
{
    uint16 ui16NeighborNum = _pLocalNodeInfo->getCount();
    RemoteNodeInfo **ppRNIs = (RemoteNodeInfo **) calloc (ui16NeighborNum+1, sizeof (RemoteNodeInfo *));
    if (ppRNIs != NULL) {
        uint16 i = 0;
        StringHashtable<Thing>::Iterator iNeighbor = _pLocalNodeInfo->iterator();
        while ( !iNeighbor.end() && (i < ui16NeighborNum)) {
            ppRNIs[i] = (RemoteNodeInfo*)iNeighbor.getValue();
            if (ppRNIs[i] != NULL) {
                i++;
            }
            iNeighbor.nextElement();
        }
    }
    return ppRNIs;
}

void WorldState::release (RemoteNodeInfo **ppRNIs)
{
    // getAllNeighborNodeInfos does not make a copy of the RemoteNodeInfo, thus
    // the elements oft e array MUST NOT be deallocated
    free (ppRNIs);
}

StringHashtable<RemoteNodeInfo> * WorldState::getAllPeerNodeInfos()
{
    _m.lock (104);
    // BE CAREFUL: this method return only the remote node infos of the alive peers
    StringHashtable<RemoteNodeInfo> *pRemoteNodeInfos = new StringHashtable<RemoteNodeInfo>();
    for (StringHashtable<Thing>::Iterator iRemoteNodes = _pNodesGraph->thingIterator(); !iRemoteNodes.end(); iRemoteNodes.nextElement()) {
        pRemoteNodeInfos->put (iRemoteNodes.getKey(), (RemoteNodeInfo*)iRemoteNodes.getValue());
    }
    _m.unlock (104);
    return pRemoteNodeInfos;
}

void WorldState::release (StringHashtable<RemoteNodeInfo> *pPtrLList)
{
    // getAllPeerNodeInfos() does not make a copy of the RemoteNodeInfo, thus
    // the elements MUST NOT be deallocated
    delete pPtrLList;
}

RemoteNodeInfo * WorldState::getPeerNodeInfo (const char *pszNodeId)
{
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pNodesGraph->get (pszNodeId);
    if (pRNI == NULL) {
        pRNI = _deadPeers.get (pszNodeId);
    }
    return pRNI;
}

bool WorldState::isPeerUpToDate (const char *pszNeighborNodeId, uint16 ui16CurrentSubscriptionStateCRC)
{
    _m.lock (105);
    RemoteNodeInfo *pRNI = getPeerNodeInfo (pszNeighborNodeId);
    if (pRNI == NULL) {
        _m.unlock (106);
        return false;
    }
    bool bRet = (pRNI->_ui16ExpectedSubscriptionStateCRC == ui16CurrentSubscriptionStateCRC);
    _m.unlock (106);
    return bRet;
}

// NODE CONFIGURATION / CONNECTION METHODS
void WorldState::incrementTopologyStateSeqId()
{
    _m.lock (107);
    _ui32TopologyStateSeqId++;
    _m.unlock (107);
}

void WorldState::newSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription)
{
    if (strNotNullAndEqual (_pDisService->getNodeId(), pszPeerNodeId)) {
        incrementSubscriptionStateSeqId();
    }
}

void WorldState::removedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription)
{
    if (strNotNullAndEqual (_pDisService->getNodeId(), pszPeerNodeId)) {
        incrementSubscriptionStateSeqId();
    }
}

void WorldState::modifiedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription)
{
    if (strNotNullAndEqual (_pDisService->getNodeId(), pszPeerNodeId)) {
        incrementSubscriptionStateSeqId();
    }
}

void WorldState::incrementSubscriptionStateSeqId()
{
    _m.lock (108);
    if (_pLocalNodeInfo == NULL) {
        _m.unlock (108);
        return;
    }
    uint32 ui32SubscriptionStateSeqId = _pLocalNodeInfo->getSubscriptionStateSequenceID();

    // Update local node's subscription state
    uint32 *pui32SeqId = _subscriptionStateTable.get (_pDisService->getNodeId());
    if (pui32SeqId == NULL) {
        pui32SeqId = new uint32[1];
        if (pui32SeqId == NULL) {
            checkAndLogMsg ("WorldState::incrementSubscriptionStateSeqId", memoryExhausted);
            _m.unlock (108);
            return;
        }
        _subscriptionStateTable.put (_pDisService->getNodeId(), pui32SeqId);
    }
    pui32SeqId[0] = ui32SubscriptionStateSeqId;

    // Update the CRC of the _ui32SubscriptionStateSeqId for each
    _crc.reset();
    for (StringHashtable<uint32>::Iterator iterator = _subscriptionStateTable.getAllElements(); !iterator.end(); iterator.nextElement()) {
        _crc.update ((const char *)iterator.getKey());
        _crc.update32 (iterator.getValue());
    }
    _ui16SubscriptionStateCRC = _crc.getChecksum();

    // The history table is changed so reset the sub state req and update the
    // sub state msg if there is any or send the new subscription
    delete _pSSReqMsg;
    _pSSReqMsg = NULL;

    PtrLList<String> *pSubscriptions = _pLocalNodeInfo->getAllSubscribedGroups();
    if (pSubscriptions != NULL) {
        sendSubscriptionStateMsg (pSubscriptions, _pDisService->getNodeId(), &ui32SubscriptionStateSeqId);
        for (String *pSub = pSubscriptions->getFirst(); pSub; pSub = pSubscriptions->getNext()) {
            delete pSub;
        }
        delete pSubscriptions;
        pSubscriptions = NULL;
    }
    _m.unlock (108);
}

void WorldState::incrementDataCacheStateSeqId ()
{
    _m.lock (109);
    _ui32DataCacheStateSeqId++;
    _m.unlock (109);
}

int WorldState::setNodeControllerTypes (const char *pszNeighborNodeId, uint8 ui8RepCtrlType, uint8 ui8FwdCtrlType)
{
    _m.lock (111);
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->getThing (pszNeighborNodeId);
    if (pRNI != NULL) {
        //pRNI->setReplicationControllerType (ui8RepCtrlType);
        //pRNI->setForwardingControllerType (ui8FwdCtrlType);
        _m.unlock (111);
        return 0;
    }
    checkAndLogMsg ("WorldState::setNodeControllerTypes", Logger::L_SevereError,
                    "no neighbor called <%s>", pszNeighborNodeId);
    _m.unlock (111);
    return -1;
}

int WorldState::updateNeighborConfiguration (const char *pszNeighborNodeId, uint8 ui8NumberOfActiveNeighbors, uint8 ui8MemorySpace, uint8 ui8Bandwidth, uint8 ui8NodesInConnectivityHistory, uint8 ui8NodesRepetitivity)
{
    _m.lock (112);
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->getThing (pszNeighborNodeId);
    if (pRNI != NULL) {
        pRNI->setNumberOfActiveNeighbors (ui8NumberOfActiveNeighbors);
        pRNI->setMemorySpace (ui8MemorySpace);
        pRNI->setBandwidth (ui8Bandwidth);
        pRNI->setNodesInConnectivityHistory (ui8NodesInConnectivityHistory);
        pRNI->setNodesRepetitivity (ui8NodesRepetitivity);

        checkAndLogMsg ("WorldState::updateNeighborConfiguration", Logger::L_MediumDetailDebug,
                        "Node: <%s>\nNumberOfActiveNeighbors = <%d>\nMemorySpace <%d>\n"
                        "Bandwidth <%d>\nConnectionHistory <%d>\nNodesRepetitivity <%d>\n",
                        pszNeighborNodeId, ui8NumberOfActiveNeighbors, ui8MemorySpace,
                        ui8Bandwidth, ui8NodesInConnectivityHistory, ui8NodesRepetitivity);
        _m.unlock (112);
        return 0;
    }
    checkAndLogMsg ("WorldState::updateNeighborConfiguration", Logger::L_SevereError,
                    "no neighbor called <%s>", pszNeighborNodeId);
    _m.unlock (112);
    return -1;
}

uint32 WorldState::getTopologyStateSeqId (void)
{
    return _ui32TopologyStateSeqId;
}

uint16 WorldState::getSubscriptionStateCRC (void)
{
    return _ui16SubscriptionStateCRC;
}

uint32 WorldState::getDataCacheStateSeqId (void)
{
    return _ui32DataCacheStateSeqId;
}

uint16 WorldState::getNumberOfActiveNeighbors()
{
    _m.lock (113);
    uint16 ui16 = _pLocalNodeInfo->getCount();
    _m.unlock (113);
    return ui16;
}

uint16 WorldState::getNumberOfActiveNeighborsWithImportance (uint8 ui8Importance, bool *pbNodesWithHigherImportance)
{
    _m.lock (114);
    *pbNodesWithHigherImportance = false;
    uint16 ui16Count = 0;
    if (_pLocalNodeInfo != NULL) {
        RemoteNodeInfo *pRNI;
        StringHashtable<Thing>::Iterator iNeighbor = _pLocalNodeInfo->iterator();
        while (!iNeighbor.end()) {
            if ((pRNI = (RemoteNodeInfo*)iNeighbor.getValue()) != NULL) {
                if (pRNI->getNodeImportance() == ui8Importance) {
                    ui16Count++;
                }
                if (!*pbNodesWithHigherImportance && (pRNI->moreImportantThan (ui8Importance))) {
                    *pbNodesWithHigherImportance = true;
                }
            }
            iNeighbor.nextElement();
        }
    }
    _m.unlock (114);
    return ui16Count;
}

char ** WorldState::getAllNeighborIPs()
{
    _m.lock (115);
    uint16 ui16NeighborNum = _pLocalNodeInfo->getCount();
    uint16 ui16Count = 0;
    char **ppszRet = new char*[ui16NeighborNum + 1];
    for (StringHashtable<Thing>::Iterator iNeighbor = _pLocalNodeInfo->iterator(); !iNeighbor.end() && (ui16Count < ui16NeighborNum); iNeighbor.nextElement()) {
        RemoteNodeInfo *pRNI = (RemoteNodeInfo*)iNeighbor.getValue();
        if (pRNI != NULL) {
            ppszRet[ui16Count] = (char *) pRNI->getDefaultIPAddressAsString();
            if (ppszRet) {
                ui16Count++;
            }
        }
    }
    ppszRet[ui16Count] = NULL;
    _m.unlock (115);
    return ppszRet;
}

char ** WorldState::getAllNeighborNodeIds()
{
    _m.lock (116);
    uint16 ui16NeighborNum = _pLocalNodeInfo->getCount();
    uint16 ui16Count = 0;
    char **ppszRet = (char**) calloc (ui16NeighborNum + 1, sizeof (char*));
    if (ppszRet == NULL) {
        checkAndLogMsg ("WorldState::getAllNeighborNodeIds", memoryExhausted);
    }
    else if (_pLocalNodeInfo != NULL) {
        StringHashtable<Thing>::Iterator iNeighbor = _pLocalNodeInfo->iterator();
        while (!iNeighbor.end() && (ui16Count < ui16NeighborNum)) {
            if ((ppszRet[ui16Count] = (char *) iNeighbor.getKey()) != NULL) {
                ui16Count++;
            }
            iNeighbor.nextElement();
        }
    }
    ppszRet[ui16Count] = NULL;
    _m.unlock (116);
    return ppszRet;
}

void WorldState::release (char **ppszIDs)
{
    // getAllNeighborNodeIds() does not make a copy of iNeighbor.getKey(), thus
    // elements MUST NOT be deallocated
    free (ppszIDs);
}

uint16 WorldState::getNumberOfActivePeers()
{
    _m.lock (117);
    uint16 ui16 = _pNodesGraph->getVertexCount();
    _m.unlock (117);
    return ui16;
}

uint8 WorldState::getMemorySpace (const char *pszNodeId)
{
    _m.lock (118);
    uint8 ui8Ret = 0;
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->getThing (pszNodeId);
    if (pRNI) {
        ui8Ret = pRNI->getMemorySpace();
    }
    _m.unlock (118);
    return ui8Ret;
}

uint8 WorldState::getBandwidth (const char *pszNodeId)
{
    _m.lock (119);
    uint8 ui8Ret = 0;
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->getThing (pszNodeId);
    if (pRNI) {
        ui8Ret = pRNI->getBandwidth();
    }
    _m.unlock (119);
    return ui8Ret;
}

uint8 WorldState::getNodesInConnectivityHistory (const char *pszNodeId)
{
    _m.lock (120);
    uint8 ui8Ret = 0;
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->getThing (pszNodeId);
    if (pRNI) {
        ui8Ret = pRNI->getNodesInConnectivityHistory();
        if (ui8Ret == 255) {
            _m.unlock (120);
            return VERY_HIGH;
        }
        else {
            _m.unlock (120);
            return (uint8) floor ((float)(ui8Ret / 51));
        }
    }
    _m.unlock (102);
    return ui8Ret;
}

uint8 WorldState::getNodeOccurrency (const char *pszNodeId)
{
    _m.lock (121);
    float fNodeOccurrency = 0;
    uint8 ui8Ret = 0;
    if (_ui32TotalNumberOfPeer == 0) {
        _m.unlock (121);
        return VERY_LOW;
    }
    else {
        float fTotalRepetitivity;
        fTotalRepetitivity = 1.0f - (float)(_ui32NumberOfNewPeer / _ui32TotalNumberOfPeer);
        RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->getThing (pszNodeId);
        if (pRNI) {
            if (pRNI->getOccurrence() == 0) {
                _m.unlock (121);
                return VERY_LOW;
            }
            fNodeOccurrency = (pRNI->getOccurrence() / _ui32TotalNumberOfPeer);
            fNodeOccurrency = (fTotalRepetitivity * 100) / fNodeOccurrency;
            if (fNodeOccurrency == 100) {
                _m.unlock (121);
                return VERY_HIGH;
            }
            // Occurrence is > 0 so return in the worst case LOW
            ui8Ret = (uint8) floor (fNodeOccurrency / 20.0) + 1;
            if (ui8Ret > 4) {
                ui8Ret = VERY_HIGH;
            }
        }
    }
    _m.unlock (121);
    return ui8Ret;
}

uint8 WorldState::getRepetitiveness (void)
{
    _m.lock (122);
    if (_ui32TotalNumberOfPeer == 0) {
        _m.unlock (122);
        return VERY_LOW;
    }
    else {
        float fNodeRepetitivity = 1.0f - (float)(_ui32NumberOfNewPeer / _ui32TotalNumberOfPeer);
        uint8 ui8ret = ((uint8)floor (fNodeRepetitivity / 0.2f));
        _m.unlock (122);
        return ui8ret;
    }
}

int WorldState::setNodeImportance (const char *pszNeighborNodeId, uint8 ui8NodeImportance)
{
    _m.lock (123);
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->getThing (pszNeighborNodeId);
    if (pRNI) {
        pRNI->setNodeImportance (ui8NodeImportance);
        _m.unlock (123);
        return 0;
    }
    checkAndLogMsg ("WorldState::setNodeImportance", Logger::L_SevereError, "no neighbor called <%s>", pszNeighborNodeId);
    _m.unlock (123);
    return -1;
}

uint8 WorldState::getNodeImportance (const char *pszNeighborNodeId)
{
    _m.lock (124);
    uint8 ui8Ret = 0;
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->getThing (pszNeighborNodeId);
    if (pRNI) {
        ui8Ret = pRNI->getNodeImportance();
    }
    _m.unlock (124);
    return ui8Ret;
}

// SUBSCRIPTION STATE
int WorldState::sendSubscriptionStateMsg (const char *pszNodeId)
{
    _m.lock (125);
    StringHashtable<DArray2<String> > *pSubscriptionsTable = NULL;
    StringHashtable<uint32> *pNodesTable = NULL;
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->get (pszNodeId);
    if (pRNI != NULL) {
        if (_pSSMsg) {
            pSubscriptionsTable = _pSSMsg->getSubscriptionsTable();
            pNodesTable = _pSSMsg->getNodesTable();
        }
        else {
            pSubscriptionsTable = new StringHashtable<DArray2<String> > (true, true, true, true);
            pNodesTable = new StringHashtable<uint32> (true, true, true, true);
        }
        StringHashtable<uint32> *pRemoteSubscriptionStateHistory = pRNI->getRemoteSubscriptionStateTable ();
        for (StringHashtable<uint32>::Iterator iterator = _subscriptionStateTable.getAllElements(); !iterator.end(); iterator.nextElement()) {
            uint32 *pui32RemoteSubscriptionSeqId = NULL;
            if (pRemoteSubscriptionStateHistory) {
                pui32RemoteSubscriptionSeqId = pRemoteSubscriptionStateHistory->get (iterator.getKey());
            }
            if (!pRemoteSubscriptionStateHistory || !pui32RemoteSubscriptionSeqId || (pui32RemoteSubscriptionSeqId < iterator.getValue())) {
                String nodeId = iterator.getKey();
                NodeInfo *pNI = (NodeInfo*)_pNodesGraph->get (nodeId);
                if (pNI) {
                    PtrLList<String> *pNIGroupNames = pNI->getAllSubscribedGroups();
                    if (pNIGroupNames && (pNIGroupNames->getFirst() != NULL)) {
                        for (String *pGroupName = pNIGroupNames->getFirst(); pGroupName != NULL; pGroupName = pNIGroupNames->getNext()) {
                            DArray2<String> *pNodeIdsArray = pSubscriptionsTable->get (pGroupName->c_str());
                            if (!pNodeIdsArray) {
                                pNodeIdsArray = new DArray2<String> (0);
                                (*pNodeIdsArray)[0] = nodeId;
                            }
                            else {
                                (*pNodeIdsArray)[pNodeIdsArray->getHighestIndex() + 1] = nodeId;
                            }
                            pSubscriptionsTable->put (pGroupName->c_str(), pNodeIdsArray);
                        }
                    }
                    uint32 *pui32SeqId = pNodesTable->get (nodeId);
                    if (!pui32SeqId) {
                        pui32SeqId = new uint32();
                    }
                    (*pui32SeqId) = *(_subscriptionStateTable.get (nodeId));
                    pNodesTable->put (nodeId, pui32SeqId);

                    for (String *pSub = pNIGroupNames->getFirst(); pSub; pSub = pNIGroupNames->getNext()) {
                        delete pSub;
                    }
                    delete pNIGroupNames;
                    pNIGroupNames = NULL;
                }
            }
        }
    }
    else {
        return -1;
    }
    if (pNodesTable->getCount() > 0) {
        _pSSMsg = new DisServiceSubscriptionStateMsg (_pDisService->getNodeId(), pSubscriptionsTable, pNodesTable);
    }
    _m.unlock (125);
    return 0;
}

int WorldState::sendSubscriptionStateMsg (PtrLList<String> *pSubscriptions, const char *pszNodeId, uint32 *pui32UpdatedSeqId)
{
    if ((pSubscriptions == NULL) || (pszNodeId == NULL) || (pui32UpdatedSeqId == NULL)) {
        return -1;
    }
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
    if (!bAreAllNeighborsUpdated) {
        StringHashtable<DArray2<String> > *pSubscriptionsTable = NULL;
        StringHashtable<uint32> *pNodesTable = NULL;
        if (_pSSMsg) {
            pSubscriptionsTable = _pSSMsg->getSubscriptionsTable();
            pNodesTable = _pSSMsg->getNodesTable();
        }
        else {
            pSubscriptionsTable = new StringHashtable<DArray2<String> > (true, true, true, true);
            pNodesTable = new StringHashtable<uint32> (true, true, true, true);
            _pSSMsg = new DisServiceSubscriptionStateMsg (_pDisService->getNodeId(), pSubscriptionsTable, pNodesTable);
        }
        uint32 *pui32SeqId = pNodesTable->get (pszNodeId);
        if ((pui32SeqId == NULL) || ((*pui32SeqId) != (*pui32UpdatedSeqId))) {
            uint32 *pui32OldSeqId = pNodesTable->remove (pszNodeId);
            delete pui32OldSeqId;
            pui32OldSeqId = NULL;
            uint32 *pui32NewSeqId = new uint32();
            (*pui32NewSeqId) = (*pui32UpdatedSeqId);
            pNodesTable->put (pszNodeId, pui32NewSeqId);
            if ((pSubscriptions != NULL) && (pSubscriptions->getFirst() != NULL)) {
                for (String *pSub = pSubscriptions->getFirst(); pSub; pSub = pSubscriptions->getNext()) {
                    DArray2<String> *pNodes = pSubscriptionsTable->get (pSub->c_str());
                    if (!pNodes) {
                        pNodes = new DArray2<String> ();
                        (*pNodes)[0] = pszNodeId;
                        pSubscriptionsTable->put (pSub->c_str(), pNodes);
                    }
                    else {
                        bool bFound = false;
                        for (int i = 0; i <= pNodes->getHighestIndex() && (!bFound); i++) {
                            if (0 == stricmp (pszNodeId, (*pNodes)[i])) {
                                bFound = true;
                            }
                        }
                        if (!bFound) {
                            (*pNodes)[pNodes->getHighestIndex() + 1] = pszNodeId;
                        }
                    }
                }
            }
        }
    }
    _m.unlock (126);
    return 0;
}

int WorldState::sendSubscriptionState (void)
{
    _m.lock (127);
    int iRet = 0;
    bool bAreAllNeighborsUpdated = true;
    if (_pSSMsg || _pSSReqMsg) {
        // If my neighbors are already updated don't send the subscription state
        for (StringHashtable<Thing>::Iterator iNeighbors = _pLocalNodeInfo->iterator(); !iNeighbors.end() && bAreAllNeighborsUpdated; iNeighbors.nextElement()) {
            RemoteNodeInfo *pRNI = (RemoteNodeInfo*) iNeighbors.getValue();
            if (pRNI != NULL) {
                if (pRNI->getExpectedSubscriptionStateCRC() != _ui16SubscriptionStateCRC) {
                    if (_pSSMsg) {
                        StringHashtable<uint32> *pNodesTable = _pSSMsg->getNodesTable();
                        // Don't send the subscriptions state if is to update the node that send the last update
                        if ((pNodesTable->getCount() != 1) || (NULL == pNodesTable->get (iNeighbors.getKey()))) {
                            bAreAllNeighborsUpdated = false;
                        }
                    }
                    else {
                        bAreAllNeighborsUpdated = false;
                    }
                }
            }
        }
        if (!bAreAllNeighborsUpdated) {
            if (_pSSMsg) {
                iRet = _pDisService->broadcastDisServiceCntrlMsg (_pSSMsg, NULL, "Sending Subscription State Message");
            }
            else {
                iRet = _pDisService->broadcastDisServiceCntrlMsg (_pSSReqMsg, NULL, "Sending SubscriptionStateReq Message");
            }
        }
        if (_pSSMsg) {
            delete (_pSSMsg->getSubscriptionsTable());
            delete (_pSSMsg->getNodesTable());
            delete _pSSMsg;
            _pSSMsg = NULL;
        }
        else {
            delete _pSSReqMsg;
            _pSSReqMsg = NULL;
        }
    }
    _m.unlock (127);
    return iRet;
}

int WorldState::sendSubscriptionStateReqMsg (DisServiceSubscriptionStateReqMsg *pSSReqMsg)
{
    _m.lock (128);
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*) _pLocalNodeInfo->get (pSSReqMsg->getTargetNodeId());
    if (pRNI != NULL && pRNI->isTopologyStateEnabled()) {
        if (!_pSSReqMsg) {
            _pSSReqMsg = pSSReqMsg;
            _pSSReqMsg->setSubscriptionStateTable (&_subscriptionStateTable);
        }
        else {
            bool bTargetSpecified = false;
            const bool isTarget = DisServiceMsgHelper::isTarget (_pSSReqMsg->getTargetNodeId(), pSSReqMsg, bTargetSpecified);
            if (bTargetSpecified && !isTarget) {
                _pSSReqMsg->setTargetNodeId (NULL);
            }
        }
    }
    _m.unlock (128);
    return 0;
}

int WorldState::receivedSubscriptionStateReqMsg (DisServiceSubscriptionStateReqMsg *pSSReqMsg)
{
    _m.lock (129);
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->get (pSSReqMsg->getSenderNodeId());
    if (pRNI != NULL) {
        pRNI->setRemoteSubscriptionStateTable (pSSReqMsg->getSubscriptionStateTable());
    }
    if (_pSSReqMsg && _pSSReqMsg->getTargetNodeId() && pSSReqMsg->getTargetNodeId()) {
        bool bTargetSpecified = false;
        const bool isTarget = DisServiceMsgHelper::isTarget (_pSSReqMsg->getTargetNodeId(), pSSReqMsg, bTargetSpecified);
        if (isTarget) {
            delete _pSSReqMsg;
            _pSSReqMsg = NULL;
        }
    }
    _m.unlock (129);
    return 0;
}

bool WorldState::isSubscriptionStateUpToDate (const char *pszNeighbprNodeId, uint16 ui16NeighborSubscriptionStateCRC)
{
    _m.lock (130);
    if (ui16NeighborSubscriptionStateCRC == 0) {
        _m.unlock (130);
        return true;
    }
    bool bRet = false;
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->get (pszNeighbprNodeId);
    if (pRNI) {
        bRet = ((pRNI->getExpectedSubscriptionStateCRC() != ui16NeighborSubscriptionStateCRC) ? false : true);
        if (!bRet && (ui16NeighborSubscriptionStateCRC == _ui16SubscriptionStateCRC)) {
            bRet = true;
            pRNI->setExpectedSubscriptionStateCRC (ui16NeighborSubscriptionStateCRC);
        }
    }
    _m.unlock (130);
    return bRet;
}

int WorldState::updateSubscriptionState (StringHashtable<DArray2<String> > *pSubscriptionsTable, StringHashtable<uint32> *pNodesTable)
{
    _m.lock (131);
    for (StringHashtable<uint32>::Iterator iterator = pNodesTable->getAllElements(); !iterator.end(); iterator.nextElement()) {
        if (0 != stricmp (_pDisService->getNodeId(), iterator.getKey())) {
            RemoteNodeInfo *pRNI = (RemoteNodeInfo*) _pNodesGraph->get (iterator.getKey());
            if (!pRNI) {
                pRNI = new RemoteNodeInfo (iterator.getKey());
                _pNodesGraph->put (iterator.getKey(), pRNI);
                pRNI->setGraph (_pNodesGraph);
                incrementTopologyStateSeqId();
            }
            uint32 *pui32RemoteSeqId = iterator.getValue();
            uint32 *pui32LocalSeqId = _subscriptionStateTable.get (iterator.getKey());
            if (!pui32LocalSeqId) {
                pui32LocalSeqId = new uint32();
            }
            if ((*pui32RemoteSeqId) > (*pui32LocalSeqId)) {
                pRNI->unsubscribeAll();
                (*pui32LocalSeqId) = (*pui32RemoteSeqId);
                _subscriptionStateTable.put (iterator.getKey(), pui32LocalSeqId);
                _crc.reset();
                for (StringHashtable<uint32>::Iterator iterator = _subscriptionStateTable.getAllElements(); !iterator.end(); iterator.nextElement()) {
                    String nodeId (iterator.getKey());
                    _crc.update (nodeId, nodeId.length());
                    _crc.update (iterator.getValue(), 4);
                }
                _ui16SubscriptionStateCRC = _crc.getChecksum();
                continue;
            }
        }
        pNodesTable->remove (iterator.getKey());
    }
    for (StringHashtable<DArray2<String> >::Iterator subIterator = pSubscriptionsTable->getAllElements(); !subIterator.end(); subIterator.nextElement()) {
        DArray2<String> *pNodes = subIterator.getValue();
        for (int i = 0; i <= pNodes->getHighestIndex(); i++) {
            String nodeId ((*pNodes)[i]);
            if (pNodesTable->get (nodeId) != NULL) {
                RemoteNodeInfo *pRNI = (RemoteNodeInfo*) _pNodesGraph->get (nodeId);
                if (!pRNI) {
                    _m.unlock (131);
                    return -1;
                }
                GroupSubscription *pGs = new GroupSubscription();
                pRNI->subscribe (subIterator.getKey(), pGs);
                PtrLList<String> *pSubscriptions =  pRNI->getAllSubscribedGroups();
                sendSubscriptionStateMsg (pSubscriptions, nodeId, pNodesTable->get (nodeId));
            }
        }
    }
    delete pSubscriptionsTable;
    pSubscriptionsTable = NULL;
    delete pNodesTable;
    pNodesTable = NULL;
    _m.unlock (131);
    return 0;
}

// FORWARDING
bool WorldState::isDirectForwarding (const char *pszGroupName)
{
    return false;
}

const char * WorldState::getProbTarget (const char *pszPeerNodeId)
{
    _m.lock (132);
    if ((_pLocalNodeInfo->get (pszPeerNodeId)) && (_pLocalNodeInfo->getCount() == 1)) {
        // The only neighbor is the one that send me the msg
        _m.unlock (132);
        return NULL;
    }
    char *pTarget = NULL;
    uint16 ui8Counter = 0;
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->get (pszPeerNodeId);
    if (pRNI) {
        ui8Counter = pRNI->getNumberOfActiveNeighbors();
        if ((ui8Counter != 0) && (ui8Counter < _pNodesGraph->getVertexCount() - 1)) {
            // Find the target node
            DArray2<String> *pMoreProbNodes = NULL;
            DArray2<String> *pLessProbNodes = NULL;
            ui8Counter = 0;
            for (StringHashtable<Thing>::Iterator iterator = _pLocalNodeInfo->getAllElements(); !iterator.end(); iterator.nextElement()) {
                if (0 != stricmp (pszPeerNodeId, iterator.getKey())) {
                    pRNI = (RemoteNodeInfo*)iterator.getValue();
                    if (pRNI) {
                        String nodeId (iterator.getKey());
                        if (ui8Counter > pRNI->getNumberOfActiveNeighbors()) {
                            if (pLessProbNodes) {
                                (*pLessProbNodes)[pLessProbNodes->getHighestIndex() + 1] = nodeId;
                            }
                            else {
                                pLessProbNodes = new DArray2<String>(0);
                                (*pLessProbNodes)[0] = nodeId;
                            }
                            continue;
                        }
                        else {
                            if (ui8Counter < pRNI->getNumberOfActiveNeighbors()) {
                                ui8Counter = pRNI->getNumberOfActiveNeighbors();
                                if (pMoreProbNodes) {
                                    if (!pLessProbNodes) {
                                        pLessProbNodes = new DArray2<String>(0);
                                        for (int i = 0; i <= pMoreProbNodes->getHighestIndex(); i++) {
                                            (*pLessProbNodes)[i] = (*pMoreProbNodes)[i];
                                        }
                                    }
                                    else {
                                        for (int i = 0; i <= pMoreProbNodes->getHighestIndex(); i++) {
                                            (*pLessProbNodes)[pLessProbNodes->getHighestIndex() + 1] = (*pMoreProbNodes)[i];
                                        }
                                    }
                                    delete pMoreProbNodes;
                                    pMoreProbNodes = NULL;
                                }
                            }
                            if (pMoreProbNodes) {
                                (*pMoreProbNodes)[pMoreProbNodes->getHighestIndex() + 1] =  nodeId;
                            }
                            else {
                                pMoreProbNodes = new DArray2<String>(0);
                                (*pMoreProbNodes)[0] = nodeId;
                            }
                        }
                    }
                }
            }
            if (pMoreProbNodes) {
                String tmp;
                DArray2<String> *pNodes = NULL;
                if ((((rand() % 100) + 1.0f) < 80) || (!pLessProbNodes)) {
                    pNodes = pMoreProbNodes;
                }
                else {
                    pNodes = pLessProbNodes;
                }
                if (pNodes->getHighestIndex() > 0) {
                    tmp = (*pNodes)[(uint8)((rand() % (pNodes->getHighestIndex() + 1)))];
                }
                else {
                    tmp = (*pNodes)[0];
                }
                pTarget = new char[tmp.length() + 1];
                memcpy (pTarget, tmp, tmp.length());
                pTarget[tmp.length()] = '\0';
            }

            delete pMoreProbNodes;
            pMoreProbNodes = NULL;
            delete pLessProbNodes;
            pLessProbNodes = NULL;
        }
    }

    _m.unlock (132);
    return pTarget;
}

PtrLList<String> * WorldState::getIsolatedNeighbors (void)
{
    PtrLList<String> *pRet = NULL;
    _m.lock (133);
    for (StringHashtable<Thing>::Iterator iNeighbors = _pLocalNodeInfo->iterator(); !iNeighbors.end(); iNeighbors.nextElement()) {
        RemoteNodeInfo *pRNI = (RemoteNodeInfo*) iNeighbors.getValue();
        if (pRNI->getNumberOfActiveNeighbors() == 1) {
            if (pRet == NULL) {
                pRet = new PtrLList<String>();
            }
            String *pNodeId = new String (pRNI->getId());
            pRet->insert (pNodeId);
        }
    }

    if (pRet != NULL) {
        int iRetNElements = ((pRet == NULL) ? 0 : pRet->getCount());
        if ((iRetNElements > 0) && (((unsigned int)iRetNElements) == _pLocalNodeInfo->getCount())) {
            // I am a node reachable only through the sender
            _m.unlock (133);

            String *pNext = pRet->getFirst();
            for (String *pCurr; (pCurr = pNext) != NULL;) {
                pNext = pRet->getNext();
                delete pRet->remove (pCurr);
            }
            delete pRet;
            return NULL;
        }
    }
    _m.unlock (133);
    return pRet;
}

float WorldState::getStatefulForwardProbability (const char *pszPeerNodeId)
{
    _m.lock (134);
    float fResult;
    uint8 ui8Counter = 1;
    RemoteNodeInfo *pRNI = (RemoteNodeInfo*)_pLocalNodeInfo->get (pszPeerNodeId);
    if (pRNI) {
        ui8Counter = pRNI->getNumberOfActiveNeighbors();
    }
    if (ui8Counter > 10) {
        ui8Counter = 10;
    }
    if (ui8Counter == 0) {
        ui8Counter = 1;
    }
    fResult = (float)(100/ui8Counter);
    _m.unlock (134);
    return fResult;
}

// Private methods
bool WorldState::isPeerAlive (RemoteNodeInfo *pRemoteNode)
{
    if (pRemoteNode && ((getTimeInMilliseconds() - pRemoteNode->getMostRecentMessageRcvdTime())
        <= _ui32DeadPeerInterval)) {
        return true;
    }
    return false;
}

void WorldState::moveChildToDeadPeers (RemoteNodeInfo *pRemoteNode)
{
    // Remove all the child not reachable
    if (pRemoteNode == NULL) {
        return;
    }
    for (StringHashtable<Thing>::Iterator iterator = pRemoteNode->iterator(); !iterator.end(); iterator.nextElement()) {
        String nodeId = iterator.getKey();
        if ((!_pLocalNodeInfo->get (nodeId)) && (nodeId != _pDisService->getNodeId()) && (!_pLocalNodeInfo->isReachable (nodeId))) {
            RemoteNodeInfo *pRNI = (RemoteNodeInfo*)iterator.getValue();
            // Remove it only if isn't a new node
            if (!isPeerAlive ((RemoteNodeInfo*)pRemoteNode->get (nodeId))) {
                pRemoteNode->remove (pRNI->getId());
                moveChildToDeadPeers (pRNI);
                if (pRNI->getCount() == 0) {
                    _pNodesGraph->remove (pRNI->getId());
                    _deadPeers.put (pRNI->getId(), pRNI);
                }
            }
        }
    }
}

