/*
 * PullReplicationController.cpp
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

#include "PullReplicationController.h"

#include "DataCacheInterface.h"
#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "DisServiceMsg.h"
#include "SQLMessageHeaderStorage.h"
#include "SubscriptionState.h"

#include "Logger.h"
#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

PullReplicationController::PullReplicationController (DisseminationService *pDisService, bool bRequireAck)
    : DataCacheReplicationController (DCRC_Pull, pDisService, bRequireAck),
      GroupMembershipService (pDisService),
      _mReplyingPeersQueue (15)
{
    _ui64LastReplication = 0;
    _ui16DataCacheQueryMsgTimer = 5000;
    _pDisService = pDisService;
}

PullReplicationController::~PullReplicationController()
{
}

void PullReplicationController::newNeighbor (const char *pszNodeUUID, const char *pszPeerRemoteAddr,
                                             const char *pszIncomingInterface)
{
    checkAndLogMsg ("PullReplicationController::newPeer", Logger::L_Info,
                    "The new peer is: <%s>\n", pszNodeUUID);
    _bReplicateDataCacheQueryMsg = true;

    addToUnreplyingPeers (pszNodeUUID);

    if (!this->isRunning()) {
    	this->start();
    }
}

void PullReplicationController::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
}

void PullReplicationController::run()
{
    setName ("PullReplicationController::run");

    // jk 3/2010 - fix so that this periodically checks if there are subscriptions
    // instead of immediately quitting if there aren't any
    started();
    String *pConstraint;
    do {
        PtrLList<String> *pSubscriptions = GroupMembershipService::getAllLocalSubscriptions();
        DisServiceDataCacheQueryMsg * pDCQMsg;
        if (pSubscriptions) {
            _mx.lock (93);
            updateUnreplyingPeers();
            // For each peer that has not answered to the DisServiceDataCacheQueryMsg
            for (String *pNodeId = _cacheQueryUnreplyingPeers.getFirst(); pNodeId; pNodeId = _cacheQueryUnreplyingPeers.getNext()) {
                const char *pszTargetNodeId = (const char *)(*pNodeId);
                // For each Subscription
                String *pGroupName = pSubscriptions->getFirst();
                if (pGroupName) {
                    // Specify the target
                    pDCQMsg = new DisServiceDataCacheQueryMsg (_pDisService->getNodeId(), pszTargetNodeId);
                    if (pDCQMsg) {
                        for (; pGroupName; pGroupName = pSubscriptions->getNext()) {
                            // NOTE: depending on the current MTU and the length
                            // of the queries, part of this message may not be
                            // sent!!!
                            // TODO: fix it!
                            _query.reset();
                            _query.selectPrimaryKey();
                            _query.addConstraintOnGroupName ((const char *)(*pGroupName));
                            pConstraint = getCondition((const char *)(*pGroupName));
                            if (pConstraint && (pConstraint->length() > 0)) {
                                _query.addCustomConstraint (pConstraint);
                            }
                            pDCQMsg->addQuery (&_query); // addQuery makes a copy
                                                         // of the query, therefore
                                                         // DisServiceDataCacheQuery's
                                                         // content can be reset
                            delete pConstraint;
                            pConstraint = NULL;
                            _query.reset();
                        }
                    }
                    _pDisService->broadcastDisServiceCntrlMsg (pDCQMsg, NULL, "Sending DisServiceDataCacheQueryMsg");
                    delete pDCQMsg;

                    Thread::yield();
                    //sleepForMilliseconds (_ui16DataCacheQueryMsgTimer);
                }
            }
            _mx.unlock (94);
        }
        sleepForMilliseconds (_ui16DataCacheQueryMsgTimer);
        pDCQMsg = NULL;
    }
    while (!terminationRequested());
    terminating();
}

void PullReplicationController::deadNeighbor (const char *pszNodeUUID)
{
    checkAndLogMsg ("PullReplicationController::deadPeer", Logger::L_Info,
                    "The dead peer is: <%s>\n", pszNodeUUID);
    addToReplyingPeers(pszNodeUUID);
}

void PullReplicationController::newLinkToNeighbor (const char *pszNodeUID,
                                                   const char *pszPeerRemoteAddr,
                                                   const char *pszIncomingInterface)
{    
}

void PullReplicationController::droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr)
{    
}

void PullReplicationController::dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad)
{
    checkAndLogMsg ("PullReplicationController::dataCacheUpdated",
                    Logger::L_Info, "the data cache has been updated.\n");
    //_ui64LastReplication = getTimeInMilliseconds ();
}

void PullReplicationController::disServiceControllerMsgArrived (ControllerToControllerMsg *pCtrlMsg)
{
	 // TODO
}

void PullReplicationController::disServiceControlMsgArrived (DisServiceCtrlMsg *pCtrlMsg)
{
    _mx.lock (95);
    if (pCtrlMsg->getType() == DisServiceMsg::DSMT_CacheEmpty) {
        addToReplyingPeers(pCtrlMsg->getSenderNodeId());
    }
    _mx.unlock (95);
}

void PullReplicationController::disServiceDataMsgArrived (DisServiceDataMsg *pDataMsg)
{
    _mx.lock (89);
    // TODO: actually we are not sure that this message has been
    // sent because the data cache query has been received.
    addToReplyingPeers(pDataMsg->getSenderNodeId());
    _mx.unlock (89);
}

void PullReplicationController::addToReplyingPeers(const char * pszNodeId)
{
    _mReplyingPeersQueue.lock (90);
    String *pNodeID = new String (pszNodeId);
    String *pNodeIDTmp = _cacheQueryUnreplyingPeers.search(pNodeID);
    if (pNodeIDTmp) {
        if (_cacheQueryReplyingPeers.search (pNodeID) == NULL) {
            _cacheQueryReplyingPeers.prepend (pNodeID);
        }
        else {
            delete pNodeID;
        }
    }
    else {
    	delete pNodeID;
    }
    _mReplyingPeersQueue.unlock (90);
}

void PullReplicationController::addToUnreplyingPeers(const char * pszNodeId)
{
    String *pNodeID = new String (pszNodeId);
    _cacheQueryUnreplyingPeers.prepend (pNodeID);
}

void PullReplicationController::updateUnreplyingPeers()
{
    _mReplyingPeersQueue.lock (91);
    String *pNodeId = _cacheQueryReplyingPeers.getFirst();
    String *pNodeIdTmp; 
    while (pNodeId) {
        pNodeIdTmp = _cacheQueryUnreplyingPeers.remove(pNodeId);
        delete pNodeIdTmp;
        pNodeIdTmp = _cacheQueryReplyingPeers.getNext();
        _cacheQueryReplyingPeers.remove(pNodeId);
        delete pNodeId;
        pNodeId = pNodeIdTmp;
    }
    _mReplyingPeersQueue.unlock (92);
}

String * PullReplicationController::getCondition (const char * pszGroupName)
{
    // The idea is creating a condition that looks like
    // nodeId NOT IN (nodeId1, nodeid2, ...) OR
    // (nodeId == nodeId1 AND msgSeqId > subscriptionStateFor_nodeid1) OR
    // (nodeId == nodeid2 AND msgSeqId > subscriptionStateFor_nodeid2)
    // ...

    DArray2<String> * pSenders = getSenderNodeIds (pszGroupName);
    if (pSenders == NULL) {
        return NULL;
    }
    SubscriptionState * pSubState = _pDisService->getSubscriptionState();
    uint32 ui32SubState;
    char * pszSeqID = new char [12];
    String notInList = "";
    String senderConds = "";
    for (int i = 0; i <= pSenders->getHighestIndex(); i++) {
        ui32SubState = pSubState->getSubscriptionState (pszGroupName, (const char *)(*pSenders)[i]);
        itoa (pszSeqID, ui32SubState);
        notInList += (String) (i > 0 ? ",'" : "'") + (*pSenders)[i] + "'";
        if (i > 0) {
            senderConds += " OR";
        }
        senderConds += (String) "(" + SQLMessageHeaderStorage::FIELD_SENDER_ID + "='" + (*pSenders)[i]
                     + "' AND " + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " > " + pszSeqID
                     + ")";
    }

    // Create final query
    String * pCond = new String ("(");
    if (notInList != "") {
        (*pCond) += (String) SQLMessageHeaderStorage::FIELD_SENDER_ID + " NOT IN (" + notInList + ")";
    }
    if (senderConds != "") {
        if (notInList != "") {
            (*pCond) += " OR ";
        }
        (*pCond) += senderConds;
    }
    (*pCond) += ")";

    return (((notInList != "") || (senderConds != "")) ? pCond : NULL);
}
