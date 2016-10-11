/*
 * TargetBasedReplicationController.cpp
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

#include "TargetBasedReplicationController.h"

#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "DataCache.h"
#include "MessageId.h"
#include "MessageInfo.h"
#include "DisServiceMsg.h"
#include "TransmissionHistoryInterface.h"
#include "ReceivedMessagesInterface.h"

#include "ConfigManager.h"
#include "StringTokenizer.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

TargetBasedReplicationController::TargetBasedReplicationController (DisseminationService *pDisService, ConfigManager *pCfgMgr)
    : DataCacheReplicationController (DCRC_TargetBased, pDisService, false),
      _replicators (false, true, true, false), _receivers (false, true, true, false), _pendingSourceNodes (false, true, true, false)
{
    _pDisService = pDisService;
    _bInitialized = false;

    _pTransmissionHistory = TransmissionHistoryInterface::getTransmissionHistory();
    if (_pTransmissionHistory == NULL) {
        checkAndLogMsg ("TargetBasedReplicationController::TargetBasedReplicationController", Logger::L_MildError,
                        "failed to get Transmission History interface\n");
        return;
    }

    _pReceivedMessages = ReceivedMessagesInterface::getReceivedMessagesInterface();
    if (_pReceivedMessages == NULL) {
        checkAndLogMsg ("TargetBasedReplicationController::TargetBasedReplicationController", Logger::L_MildError,
                        "failed to get Received Messages interface\n");
        return;
    }

    if (configure (pCfgMgr) != 0) {
        checkAndLogMsg ("TargetBasedReplicationController::TargetBasedReplicationController", Logger::L_MildError,
                        "configuration failed\n");
        return;
    }
    _bInitialized = true;
}

TargetBasedReplicationController::~TargetBasedReplicationController()
{
}

int TargetBasedReplicationController::configure (ConfigManager *pCfgMgr)
{
    const char *pszMethod = "TargetBasedReplicationController::configure";

    // Load the target specifications from config file
    const char *pszIncludedTargets = pCfgMgr->getValue ("aci.disService.replicationTarget.includeList");
    const char *pszExcludedTargets = pCfgMgr->getValue ("aci.disService.replicationTarget.excludeList");

    // Parse the targets into arrays
    if (pszIncludedTargets) {
        StringTokenizer tokenizer (pszIncludedTargets, ';');
        const char *pszToken;
        while ((pszToken = tokenizer.getNextToken()) != NULL) {
            String *pTargetId = new String (pszToken);
            _targetIncludeSpec.append (pTargetId);
            checkAndLogMsg (pszMethod, Logger::L_Info,
                            "added %s as an included target specification\n", pszToken);
        }
    }
    if (pszExcludedTargets) {
        StringTokenizer tokenizer (pszExcludedTargets, ';');
        const char *pszToken;
        while ((pszToken = tokenizer.getNextToken()) != NULL) {
            String *pTargetId = new String (pszToken);
            _targetExcludeSpec.append (pTargetId);
            checkAndLogMsg (pszMethod, Logger::L_Info,
                            "added %s as an excluded target specification\n", pszToken);
        }
    }

    // Determine whether the target's existing messages should be used as a filter
    _bCheckTargetForMsgs = pCfgMgr->getValueAsBool ("aci.disService.replication.checkTargetForMsgs", true);
    checkAndLogMsg (pszMethod, Logger::L_Info,
                    "check target's existing messages for filtration: %s\n",
                    _bCheckTargetForMsgs ? "true" : "false");

    // Check whether replicated messages should be acknowledged
    _bRequireAcks = pCfgMgr->getValueAsBool ("aci.disService.replication.requireAcks", true);
    checkAndLogMsg (pszMethod, Logger::L_Info,
                    "require acknowledgements: %s\n",
                    _bRequireAcks ? "true" : "false");

    // Check whether we should perform concurrent replication (to multiple target peers)
    _bAllowConcurrentReplication = pCfgMgr->getValueAsBool ("aci.disService.replication.concurrentReplication", false);
    checkAndLogMsg (pszMethod, Logger::L_Info,
                    "allow concurrent replication to multiple targets: %s\n",
                    _bAllowConcurrentReplication ? "true" : "false");

    // Check whether we should receive from multiple replications concurrently
    _bAllowConcurrentReception = pCfgMgr->getValueAsBool ("aci.disService.replication.concurrentReception", false);
    checkAndLogMsg (pszMethod, Logger::L_Info,
                    "allow concurrent reception from multiple replicators: %s\n",
                    _bAllowConcurrentReception ? "true" : "false");
    return 0;
}

bool TargetBasedReplicationController::evaluatePeerForReplication (const char *pszNodeUUID)
{
    // To be selected as a target, the node should match one of the specifications in the include list and
    // must NOT match any of the specifications in the excludeList
    bool bFound = false;
    String *pSpec;
    _targetIncludeSpec.resetGet();
    while (NULL != (pSpec = _targetIncludeSpec.getNext())) {
        if (wildcardStringCompare (pszNodeUUID, pSpec->c_str())) {
            bFound = true;
            break;
        }
    }
    if (!bFound) {
        return false;
    }
    _targetExcludeSpec.resetGet();
    while (NULL != (pSpec = _targetExcludeSpec.getNext())) {
        if (wildcardStringCompare (pszNodeUUID, pSpec->c_str())) {
            bFound = false;
            break;
        }
    }
    return bFound;
}

/*
  dequeue nextNode from _pendingDestNodes
      if nextNode is not NULL
          if nextNode is not a deadPeer
               create new replication session
               add this node to the active _replicators list
*/
int TargetBasedReplicationController::checkAndStartNextReplicationSession (void)
{
    const char *pszMethod = "TargetBasedReplicationController::checkAndStartNextReplicationSession";

    _mPendingDestNodes.lock();
    if (_pendingDestNodes.isEmpty()) {
        _mPendingDestNodes.unlock();
        return 0;
    }
    if (_bAllowConcurrentReplication) {
        String *pTargetNodeID;
        _mReplicators.lock();
        while (NULL != (pTargetNodeID = _pendingDestNodes.dequeue())) {
            ReplicationSessionThread *pRST = new ReplicationSessionThread (this);
            pRST->init (_pDisService->getNodeId(), pTargetNodeID->c_str(), _bCheckTargetForMsgs, _bRequireAcks);
            _replicators.put (pTargetNodeID->c_str(), pRST);
            checkAndLogMsg (pszMethod, Logger::L_Info,
                            "starting a replication session to node %s\n",
                            pTargetNodeID->c_str());
            pRST->start();
        }
        _mReplicators.unlock();
    }
    else {
        _mReplicators.lock();
        if (_replicators.getCount() > 0) {
            // Can only have one active replicator
            _mPendingDestNodes.unlock();
            _mReplicators.unlock();
            return 0;
        }
        String *pTargetNodeID = _pendingDestNodes.dequeue();
        ReplicationSessionThread *pRST = new ReplicationSessionThread (this);
        pRST->init (_pDisService->getNodeId(), pTargetNodeID->c_str(), _bCheckTargetForMsgs, _bRequireAcks);
        _replicators.put (pTargetNodeID->c_str(), pRST);
        checkAndLogMsg (pszMethod, Logger::L_Info,
                        "starting a sequential replication session to node %s\n",
                        pTargetNodeID->c_str());
        pRST->start();
        _mReplicators.unlock();
    }
    _mPendingDestNodes.unlock();
    return 0;
}

/*
  dequeue nextNode from _pendingSourceNodes
      if nextNode is not NULL
          if nextNode is not a deadPeer
               put nextNode into active _receivers list
               send INFO_REPSONSE to nextNode
*/
int TargetBasedReplicationController::checkAndStartNextReceivingSession (void)
{
    const char *pszMethod = "TargetBasedReplicationController::checkAndStartNextReceivingSession";

    _mPendingSourceNodes.lock();
    if (_pendingSourceNodes.getCount() == 0) {
        _mPendingSourceNodes.unlock();
        return 0;
    }
    if (_bAllowConcurrentReception) {
        _mReceivers.lock();
        for (StringHashtable<ReceiverSession>::Iterator i = _pendingSourceNodes.getAllElements(); !i.end(); i.nextElement()) {
            const char *pszSenderNodeID = i.getKey();
            ReceiverSession *pRS = i.getValue();
            _receivers.put (pszSenderNodeID, pRS);
            checkAndLogMsg (pszMethod, Logger::L_Info,
                            "starting to receive replication data from node %s\n",
                            pszSenderNodeID);
            pRS->startReceiving();
        }
        _pendingSourceNodes.removeAll();
        _mReceivers.unlock();
    }
    else {
        _mReceivers.lock();
        if (_receivers.getCount() > 0) {
            // Can only have one active receiver
            _mPendingSourceNodes.unlock();
            _mReceivers.unlock();
            return 0;
        }
        StringHashtable<ReceiverSession>::Iterator i = _pendingSourceNodes.getAllElements();
        if (!i.end()) {
            const char *pszSenderNodeID = i.getKey();
            ReceiverSession *pRS = i.getValue();
            _receivers.put (pszSenderNodeID, pRS);
            checkAndLogMsg (pszMethod, Logger::L_Info,
                            "starting to sequentially receive replication data from node %s\n",
                            pszSenderNodeID);
            pRS->startReceiving();
            _pendingSourceNodes.remove (pszSenderNodeID);
        }
        _mReceivers.unlock();
    }
    _mPendingSourceNodes.unlock();
    return 0;
}

bool TargetBasedReplicationController::cleanupTerminatedReplicators (void)
{
    /*_mTerminatingReplicators.lock();
    bool bDone = false;
    bool bComplete = true;
    while (!bDone) {
        bDone = true;
        _terminatingReplicators.resetGet();
        ReplicationSessionThread *pRST;
        while (NULL != (pRST = _terminatingReplicators.getNext())) {
            if (pRST->hasTerminated()) {
                _terminatingReplicators.remove (pRST);
                delete pRST;
                bDone = false;
                break;
            }
            else {
                bComplete = false;
            }
        }
    }
    _mTerminatingReplicators.unlock();
    return bComplete; */
    return true;
}

void TargetBasedReplicationController::newNeighbor (const char *pszNodeUUID,
                                                    const char *pszPeerRemoteAddr,
                                                    const char *pszIncomingInterface)
{
    const char *pszMethod = "TargetBasedReplicationController::newNeighbor";
    checkAndLogMsg (pszMethod, Logger::L_Info,
                    "the new peer is <%s>\n", pszNodeUUID);

    // Target check
    if (!evaluatePeerForReplication (pszNodeUUID)) {
        checkAndLogMsg (pszMethod, Logger::L_Info,
                        "skipping peer <%s> for replication\n", pszNodeUUID);
        return;
    }

    // Do some error checking first
    // Check if new neighbor is currently a replication target
    ReplicationSessionThread *pRST;
    _mReplicators.lock();
    if (NULL != (pRST = _replicators.get (pszNodeUUID))) {
        // Currently replicating to this node already!
        checkAndLogMsg (pszMethod, Logger::L_Warning,
                        "received <%s> as a new neighbor - but already have an active replication session to this node - ignoring\n",
                        pszNodeUUID);
        _mReplicators.unlock();
        return;
    }
    _mReplicators.unlock();

    // Check if new neighbor is currently awaiting replication
    String *pNodeUUID;
    _mPendingDestNodes.lock();
    _pendingDestNodes.resetGet();
    while (NULL != (pNodeUUID = _pendingDestNodes.getNext())) {
        if (0 == stricmp (pNodeUUID->c_str(), pszNodeUUID)) {
            // Already in the pending node list!
            checkAndLogMsg (pszMethod, Logger::L_Warning,
                            "received <%s> as a new neighbor - but already have this node in the pending list - ignoring\n",
                            pszNodeUUID);
            _mPendingDestNodes.unlock();
            return;
        }
    }
    
    // Ok - Add this node to the pending list
    String *pDestNodeID = new String (pszNodeUUID);
    _pendingDestNodes.enqueue (pDestNodeID);
    _mPendingDestNodes.unlock();
    checkAndStartNextReplicationSession();
}

/*
    Lost contact with a neighborNode

    if there was an active replication session with the neighborNode
       terminate the session and remove it from the active list
       checkAndStartNextReplicationSession
    if the neighborNode was serially queued in _pendingDestNodes
       remove it from the queue

    if there was an active receiver session with the neighborNode
       terminate the session and remove it from the active list
       checkAndStartNextReceivingSession
    if the neighborNode was serially queued in _mPendingSourceNodes
       remove it from the queue
    
*/
void TargetBasedReplicationController::deadNeighbor (const char *pszNodeUUID)
{
    const char *pszMethod = "TargetBasedReplicationController::deadNeighbor";
    checkAndLogMsg (pszMethod, Logger::L_Info,
                    "the dead neighbor is: <%s>\n", pszNodeUUID);

    // Check if there is an active replication session ongoing with this node
    ReplicationSessionThread *pRST;
    _mReplicators.lock();
    while (NULL != (pRST = _replicators.get (pszNodeUUID))) {
        // Currently replicating to this node - need to end this session
        checkAndLogMsg (pszMethod, Logger::L_Info,
                        "received <%s> as a dead neighbor - must terminate active replication session to this node\n", pszNodeUUID);
        pRST->targetDied();
        _replicators.remove (pszNodeUUID);
        _terminatingReplicators.append (pRST);
        _mReplicators.unlock();
        checkAndStartNextReplicationSession();
        return;
    }

    ReceiverSession *pRS;
    _mReceivers.lock();
    if (NULL != (pRS = _receivers.remove (pszNodeUUID))) {
        // Currently receiving from this node - need to terminate this session
        checkAndLogMsg (pszMethod, Logger::L_Info,
                        "received <%s> as a dead neighbor - must terminate active receiver session from this node\n", pszNodeUUID);
        delete pRS;     // NOTE: Do not call sessionEnding() on ReceiverSession as there is no point in sending the final ACKs
        _mReplicators.unlock();
        _mReceivers.unlock();
        checkAndStartNextReceivingSession();
        return;
    }
    _mReceivers.unlock();

    // Check if this node is in one of the pending lists
    String nodeUUID (pszNodeUUID);
    _mPendingDestNodes.lock();
    if (_pendingDestNodes.remove (&nodeUUID) != NULL) {
        checkAndLogMsg (pszMethod, Logger::L_Info,
                        "removed <%s> from the list of pending destination nodes\n", pszNodeUUID);
    }
    _mPendingDestNodes.unlock();

    _mPendingSourceNodes.lock();
    if (NULL != (pRS = _pendingSourceNodes.remove (pszNodeUUID))) {
        checkAndLogMsg (pszMethod, Logger::L_Info,
                        "removed <%s> from the list of pending source nodes\n", pszNodeUUID);
        delete pRS;
    }
    _mPendingSourceNodes.unlock();

    _mReplicators.unlock();

}

/*
  if more than one link supported between neighbors

  NOT SUPPORTED
*/
void TargetBasedReplicationController::newLinkToNeighbor (const char *pszNodeUID,
                                                          const char *pszPeerRemoteAddr,
                                                          const char *pszIncomingInterface)
{
}

void TargetBasedReplicationController::droppedLinkToNeighbor (const char *pszNodeUID,
                                                              const char *pszPeerRemoteAddr)
{    
}

/*
 * Keep track of complete messages that have been received - used to generate ReplicationStartReply message
 */
void TargetBasedReplicationController::dataCacheUpdated (MessageHeader *pMH, const void *pPayload)
{
    if ((pMH == NULL) || (!pMH->isCompleteMessage())) {
        // Only care about complete messages that have been added to the data cache
        return;
    }

    int rc = _pReceivedMessages->addMessage (pMH);
    if (rc < 0) {
        checkAndLogMsg ("TargetBasedReplicationController::dataCacheUpdated", Logger::L_Info,
                        "insert of message %s in ReceivedMessages failed with rc = %d\n",
                        pMH->getMsgId(), rc);
    }
    else {
        checkAndLogMsg ("TargetBasedReplicationController::dataCacheUpdated", Logger::L_Info,
                        "added message %s to ReceivedMessages\n",pMH->getMsgId());
    }

    // Notify any active receiver session so that they may generate acknowledgements if necessary
    _mReceivers.lock();
    for (StringHashtable<ReceiverSession>::Iterator i = _receivers.getAllElements(); !i.end(); i.nextElement()) {
        ReceiverSession *pRS = i.getValue();
        pRS->dataCacheUpdated (pMH, pPayload);
    }
    _mReceivers.unlock();
}

void TargetBasedReplicationController::disServiceControllerMsgArrived (ControllerToControllerMsg *pCtrlMsg)
{
    const char *pszMethod = "TargetBasedReplicationController::disServiceControllerMsgArrived";

    if (pCtrlMsg->getControllerType() != CONTROLLER_TYPE) {
        // Not for me
        return;
    }

    const char *pszCtrlMsgSenderID = pCtrlMsg->getSenderNodeId();
    if (pszCtrlMsgSenderID == NULL) {
        checkAndLogMsg (pszMethod, Logger::L_Info, 
                        "failed to retrieve message sender ID\n");
        return;
    }

    if (pCtrlMsg->getControllerVersion() != CONTROLLER_VERSION) {
        checkAndLogMsg (pszMethod, Logger::L_Warning,
                        "TargetBasedReplicationController version mismatch with node %s - ignoring\n", pszCtrlMsgSenderID);
        return;
    }

    uint32 ui32MetadataLen = pCtrlMsg->getMetaDataLength();
    if (ui32MetadataLen == 0) {
        checkAndLogMsg (pszMethod, Logger::L_Info, 
                        "failed to retrieve metadata length\n");
        return;
    }

    uint8 *pui8MetaData = (uint8 *) pCtrlMsg->getMetaData();
    if (pui8MetaData == NULL) {
        checkAndLogMsg (pszMethod, Logger::L_Info, 
                        "failed to retrieve metadata\n");
        return;
    }

    switch (*pui8MetaData) {
        case ControllerToControllerMsg::DSCTCMT_RepStartReq:
        {
            // Received a new request for a replication session - check to make sure no entry exists for this node alread
            _mReceivers.lock();
            _mPendingSourceNodes.lock();
            ReceiverSession *pRS;
            if (NULL != (pRS = _receivers.get  (pszCtrlMsgSenderID))) {
                checkAndLogMsg (pszMethod, Logger::L_Warning,
                                "received a RepStartReq from %s, for which there is already a receiver session - deleting old session\n", pszCtrlMsgSenderID);
                _receivers.remove (pszCtrlMsgSenderID);
                delete pRS;
            }
            if (NULL != (pRS = _pendingSourceNodes.get (pszCtrlMsgSenderID))) {
                checkAndLogMsg (pszMethod, Logger::L_Warning,
                                "received a RepStartReq from %s, which was already in the list of pending nodes - deleting old request\n", pszCtrlMsgSenderID);
                _pendingSourceNodes.remove (pszCtrlMsgSenderID);
                delete pRS;
            }

            // Create a new ReceiverSession for this request
            ReplicationStartReqMsg rsrm (pCtrlMsg);
            pRS = new ReceiverSession (this);
            pRS->init (_pDisService->getNodeId(), pszCtrlMsgSenderID, rsrm.sendCurrentDataList(), rsrm.requireAcks());
            _pendingSourceNodes.put (pszCtrlMsgSenderID, pRS);
            _mPendingSourceNodes.unlock();
            _mReceivers.unlock();
            checkAndStartNextReceivingSession();
            return;
        }

        case ControllerToControllerMsg::DSCTCMT_RepStartReply:
        {
            _mReplicators.lock();
            ReplicationSessionThread *pRST = _replicators.get (pszCtrlMsgSenderID);
            if (pRST == NULL) {
                checkAndLogMsg (pszMethod, Logger::L_Warning,
                                "received a RepStartReply from %s, which is not in the list of replicators - ignoring\n", pszCtrlMsgSenderID);
                _mReplicators.unlock();
                return;
            }
            ReplicationStartReplyMsg rsrm (pCtrlMsg);
            pRST->replyMsgArrived (&rsrm);
            _mReplicators.unlock();
            return;
        }

        case ControllerToControllerMsg::DSCTCMT_RepEnd:
        {
            _mReceivers.lock();
            ReceiverSession *pRS = _receivers.get (pszCtrlMsgSenderID);
            if (pRS == NULL) {
                checkAndLogMsg (pszMethod, Logger::L_Warning,
                                "received a RepEnd message from %s for which there is no active receiver session\n", pszCtrlMsgSenderID);
            }
            else {
                _receivers.remove (pszCtrlMsgSenderID);
                pRS->sessionEnding();
                delete pRS;
                checkAndLogMsg (pszMethod, Logger::L_Info,
                                "received a RepEnd message from %s - ending session\n", pszCtrlMsgSenderID);
            }
            _mReceivers.unlock();
            checkAndStartNextReceivingSession();
            return;
        }

        case ControllerToControllerMsg::DSCTCMT_RepAck:
        {
            ReplicationAckMsg ram (pCtrlMsg);
            PtrQueue<String> *pMsgIDs = ram.getMsgIDs();
            if ((pMsgIDs != NULL) && (!pMsgIDs->isEmpty())) {
                // Add replicated message to transmission history
                String *pMsgID;
                while (NULL != (pMsgID = pMsgIDs->dequeue())) {
                    _pTransmissionHistory->addMessageTarget (pMsgID->c_str(), pszCtrlMsgSenderID);
                    checkAndLogMsg (pszMethod, Logger::L_LowDetailDebug,
                                    "received acknowledgement that message %s was replicated to destination node %s\n",
                                    pMsgID->c_str(), pszCtrlMsgSenderID);
                    delete pMsgID;
                }
            }
            return;
        }
    }
}

void TargetBasedReplicationController::disServiceDataMsgArrived (DisServiceDataMsg *pDataMsg)
{
    // NOTE: This method used to notify the ReceiverSession, but that is no longer being done here
    //       Instead, it is done when the Data Cache is updated
}

/*
  not using this controller to controller disservice function
*/
void TargetBasedReplicationController::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
}

/*
non controller to controller higher level disservice messages not used
*/
void TargetBasedReplicationController::disServiceControlMsgArrived (DisServiceCtrlMsg *pCtrlMsg)
{
}

void TargetBasedReplicationController::addTerminatingReplicator (ReplicationSessionThread *pRST)
{
    cleanupTerminatedReplicators();
    _mReplicators.lock();
    _mTerminatingReplicators.lock();
    if (pRST != _replicators.remove (pRST->getTargetNodeID())) {
        checkAndLogMsg ("TargetBasedReplicationController::addTerminatingReplicator", Logger::L_MildError,
                        "inconsistency in replicators hashtable\n");
    }
    _terminatingReplicators.append (pRST);
    _mTerminatingReplicators.unlock();
    _mReplicators.unlock();
    checkAndStartNextReplicationSession();
}

// Methods from ReplicationSessionThread - an inner class of TargetBasedReplicationController

TargetBasedReplicationController::ReplicationSessionThread::ReplicationSessionThread (TargetBasedReplicationController *pTBRepCtlr)
    : _cv (&_m)
{
    _pTBRepCtlr = pTBRepCtlr;
    _bCheckTargetForMsgs = false;
    _bRequireAcks = false;
    _bReplicating = false;
    _bTargetDead = false;
    _pMsgsToExclude = NULL;
}

TargetBasedReplicationController::ReplicationSessionThread::~ReplicationSessionThread (void)
{
    if (_pMsgsToExclude != NULL) {
        delete _pMsgsToExclude;
        _pMsgsToExclude = NULL;
    }
}

int TargetBasedReplicationController::ReplicationSessionThread::init (const char *pszLocalNodeID, const char *pszTargetNodeID, bool bCheckTargetForMsgs, bool bRequireAcks)
{
    if ((pszLocalNodeID == NULL) || (strlen (pszLocalNodeID) <= 0)) {
        return -1;
    }
    if ((pszTargetNodeID == NULL) || (strlen (pszTargetNodeID) <= 0)) {
        return -2;
    }
    _localNodeID = pszLocalNodeID;
    _targetNodeID = pszTargetNodeID;
    _bCheckTargetForMsgs = bCheckTargetForMsgs;
    _bRequireAcks = bRequireAcks;
    return 0;
}


/*
   send ReplicationStartReqMsg
   if send error delete self
   wait for ReplicationStartReplyMsg as follows
      loop until reply is received
          on receive error or peerNode dies, delete self
   get filtered list of messages to replicate
   for all messages to send, loop
       if clear to send
          send next message
          if send error, delete self
       else wait for clear to send (watch out for deadPeer?)
    all done delete self

    note: deleting self signals thread's parent to terminate this thread
          go through the list of successfully send and ack'd?
          and possibly queue up another session?
              
*/
void TargetBasedReplicationController::ReplicationSessionThread::run (void)
{
    int rc;
    const char *pszMethod = "TargetBasedReplicationController::ReplicationSessionThread::run";

    // First - send the ReplicationStartReqMsg
    ReplicationStartReqMsg rsrm (_localNodeID, _targetNodeID, TargetBasedReplicationController::CONTROLLER_TYPE, TargetBasedReplicationController::CONTROLLER_VERSION, _bCheckTargetForMsgs, _bRequireAcks);

    if (_pTBRepCtlr->transmitCtrlToCtrlMessage (&rsrm, "TargetBasedReplicationController: sending ReplicationStartReqMsg") != 0) {
        checkAndLogMsg (pszMethod, Logger::L_MildError,
                        "transmission of ReplicationStartReqMsg failed - terminating replication session\n");
        _pTBRepCtlr->addTerminatingReplicator (this);
        setTerminatingResultCode (-1);
        terminating();
        return;
    }
    else {
        checkAndLogMsg (pszMethod, Logger::L_Info,
                        "transmitted ReplicationStartReqMsg to target node %s\n",
                        (const char*) _targetNodeID);
    }

    // Wait for the ReplicationStartReplyMsg
    _m.lock();
    while (!_bReplicating) {
        if (terminationRequested()) {
            _pTBRepCtlr->addTerminatingReplicator (this);
            setTerminatingResultCode (-2);
            terminating();
            _m.unlock();
            return;
        }
        else if (_bTargetDead) {
            checkAndLogMsg (pszMethod, Logger::L_Warning,
                            "target node %s died while waiting for the ReplicationStartReplyMsg\n",
                            (const char*) _targetNodeID);
            _pTBRepCtlr->addTerminatingReplicator (this);
            setTerminatingResultCode (-3);
            terminating();
            _m.unlock();
            return;
        }
        _cv.wait (1000);
    }
    _m.unlock();

    // Ready to replicate messages at this point
    if ((_pMsgsToExclude != NULL) && (_pMsgsToExclude->getCount() > 0)) {
        checkAndLogMsg (pszMethod, Logger::L_Info,
                        "adding constraint to exclude messages previously received by node %s\n", (const char*) _targetNodeID);
    }

    int64 i64QueryStartTime = getTimeInMilliseconds();
    PtrLList<MessageId> *pMsgIds = _pTBRepCtlr->_pDataCacheInterface->getNotReplicatedMsgList (_targetNodeID, 0, _pMsgsToExclude);
    if (pMsgIds != NULL) {
        checkAndLogMsg (pszMethod, Logger::L_Info,
                        "identified %lu messages to replicate to node %s - query time %lu ms\n",
                        (uint32) pMsgIds->getCount(), (const char*) _targetNodeID, (uint32) (getTimeInMilliseconds() - i64QueryStartTime));
        for (MessageId *pMIdTemp = pMsgIds->getFirst(); pMIdTemp; pMIdTemp = pMsgIds->getNext()) {
            // Check for termination
            if (terminationRequested()) {
                setTerminatingResultCode (-4);
                break;
            }

            // Check Flow Control and target peer death
            int64 i64PauseStartTime = 0;
            while ((!_bTargetDead) && (!_pTBRepCtlr->clearToSendOnAllInterfaces())) {
                if (i64PauseStartTime == 0) {
                    i64PauseStartTime = getTimeInMilliseconds();
                }
                sleepForMilliseconds (100);
            }
            if (_bTargetDead) {
                checkAndLogMsg (pszMethod, Logger::L_Warning,
                                "target %s died while replicating messages\n",
                                (const char*) _targetNodeID);
                setTerminatingResultCode (-5);
                break;
            }
            if (i64PauseStartTime != 0) {
                checkAndLogMsg (pszMethod, Logger::L_Info,
                                "paused replication for %lu ms because it was not clear to send\n",
                                (uint32) (getTimeInMilliseconds() - i64PauseStartTime));
            }

            // Replicate one message
            if (0 != (rc = _pTBRepCtlr->replicateMessage (pMIdTemp->getId(), _targetNodeID, 2000))) {
                checkAndLogMsg (pszMethod, Logger::L_Warning,
                                "replicateMessage() failed for message <%s> to target node <%s>; rc = %d\n",
                                pMIdTemp->getId(), (const char*) _targetNodeID, rc);
            }
            else if (!_bRequireAcks) {
                // Assume that the message has been successfully replicated
                _pTBRepCtlr->_pTransmissionHistory->addMessageTarget (pMIdTemp->getId(), _localNodeID);
                checkAndLogMsg (pszMethod, Logger::L_LowDetailDebug,
                                "replicated message %s to destination node %s\n",
                                pMIdTemp->getId(), (const char *) _localNodeID);
            }
        }
    }
    delete pMsgIds;
    _pTBRepCtlr->releaseQueryResults();

    // Send the ReplicationEnd message
    if (_bTargetDead) {
            checkAndLogMsg (pszMethod, Logger::L_MildError,
                            "not sending replication end message because target node %s is dead\n",
                            (const char *) _targetNodeID);
    }
    else {
        ReplicationEndMsg rem (_localNodeID, _targetNodeID, TargetBasedReplicationController::CONTROLLER_TYPE, TargetBasedReplicationController::CONTROLLER_VERSION);
        if (0 != (rc = _pTBRepCtlr->transmitCtrlToCtrlMessage (&rem, "replication end message"))) {
            checkAndLogMsg (pszMethod, Logger::L_MildError,
                            "failed to send replication end message; rc = %d\n", rc);
        }
        else {
            checkAndLogMsg (pszMethod, Logger::L_Info,
                            "completed replication session with target node %s\n",
                            (const char *) _targetNodeID);
        }
    }

    _pTBRepCtlr->addTerminatingReplicator (this);
    terminating();
}

const char * TargetBasedReplicationController::ReplicationSessionThread::getTargetNodeID (void)
{
    return _targetNodeID;
}

int TargetBasedReplicationController::ReplicationSessionThread::targetDied (void)
{
    _m.lock();
    _bTargetDead = true;  // Tell run loop that peer has died
    _cv.notifyAll();
    _m.unlock();
    return 0;
}

int TargetBasedReplicationController::ReplicationSessionThread::replyMsgArrived (ReplicationStartReplyMsg *pReplyMsg)
{
    _m.lock();
    _pMsgsToExclude = pReplyMsg->relinquishReceivedMsgsList();
    _bReplicating = true;  // Tell run loop that reply has been received
    _cv.notifyAll();
    _m.unlock();
    return 0;
}

bool TargetBasedReplicationController::ReplicationSessionThread::operator == (const ReplicationSessionThread &rhsRST)
{
    return ((_targetNodeID == rhsRST._targetNodeID) != 0);
}


// Methods from ReceiverSession - an inner class of TargetBasedReplicationController

TargetBasedReplicationController::ReceiverSession::ReceiverSession (TargetBasedReplicationController *pTBRepCtlr)
{
    _pTBRepCtlr = pTBRepCtlr;
    _i64LastAckTime = 0;
}

TargetBasedReplicationController::ReceiverSession::~ReceiverSession (void)
{
}

int TargetBasedReplicationController::ReceiverSession::init (const char *pszLocalNodeID, const char *pszSourceNodeID, bool bSendCurrentDataList, bool bGenerateAcks)
{
    if ((pszLocalNodeID == NULL) || (strlen (pszLocalNodeID) <= 0)) {
        return -1;
    }
    if ((pszSourceNodeID == NULL) || (strlen (pszSourceNodeID) <= 0)) {
        return -2;
    }
    _localNodeID = pszLocalNodeID;
    _sourceNodeID = pszSourceNodeID;
    _bSendCurrentDataList = bSendCurrentDataList;
    _bGenerateAcks = bGenerateAcks;
    _i64LastAckTime = getTimeInMilliseconds();
    return 0;
}

int TargetBasedReplicationController::ReceiverSession::startReceiving (void)
{
    StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pReceivedMsgs = NULL;
    if (_bSendCurrentDataList) {
        pReceivedMsgs = _pTBRepCtlr->_pReceivedMessages->getReceivedMsgsByGrpPub();
    }
    int rc;
    ReplicationStartReplyMsg rsrm (_localNodeID, _sourceNodeID, TargetBasedReplicationController::CONTROLLER_TYPE, TargetBasedReplicationController::CONTROLLER_VERSION, pReceivedMsgs);
    if (0 != (rc = _pTBRepCtlr->transmitCtrlToCtrlMessage (&rsrm, "TargetBasedReplicationController sending start reply message"))) {
        checkAndLogMsg ("TargetBasedReplicationController::ReceiverSession::startReceiving", Logger::L_MildError,
                        "failed to transmit start receiving message; rc = %d\n", rc);
    }
    return 0;
}

void TargetBasedReplicationController::ReceiverSession::dataCacheUpdated (MessageHeader *pMH, const void *pPayload)
{
    // TargetBasedReplicationController has already filtered out cases where this is not a complete message
    if (_bGenerateAcks) {
        String *pMsgID = new String (pMH->getMsgId());
        _msgsToAck.enqueue (pMsgID);
        if ((_msgsToAck.size() > DEFAULT_NUM_OF_MSGS_TO_ACK) || ((getTimeInMilliseconds()-_i64LastAckTime) > DEFAULT_ACK_TIMEOUT)) {
            sendAckMsg();
            _i64LastAckTime = getTimeInMilliseconds();
        }
    }
}

void TargetBasedReplicationController::ReceiverSession::sessionEnding (void)
{
    if (_msgsToAck.size() > 0) {
        sendAckMsg();
        _i64LastAckTime = getTimeInMilliseconds();
    }
}

bool TargetBasedReplicationController::ReceiverSession::operator == (const ReceiverSession &rhsRS)
{
    return ((_sourceNodeID == rhsRS._sourceNodeID) != 0);
}

void TargetBasedReplicationController::ReceiverSession::sendAckMsg (void)
{
    int rc;
    ReplicationAckMsg ram (_localNodeID, _sourceNodeID, TargetBasedReplicationController::CONTROLLER_TYPE, TargetBasedReplicationController::CONTROLLER_VERSION, &_msgsToAck);
    if (0 != (rc = _pTBRepCtlr->transmitCtrlToCtrlMessage (&ram, "TargetBasedReplicationController acknowledging receipt of DataMsg"))) {
        checkAndLogMsg ("TargetBasedReplicationController::ReceiverSession::dataMsgArrived", Logger::L_MildError,
                        "failed to transmit acknowledgement message; rc = %d\n", rc);
    }
}
