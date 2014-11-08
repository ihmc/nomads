/*
 * AckController.cpp
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

#include "AckController.h"

#include "DataCacheReplicationController.h"
#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "DisServiceMsg.h"

#include "Logger.h"
#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const uint32 AckController::DEFAULT_RETRANSMISSION_TIMEOUT = 5000;
const uint32 AckController::DEFAULT_RETRANSMISSION_CYCLE_PERIOD = 5000;
const uint8 AckController::DEFAULT_TRANSMISSION_WINDOW = 5;
const bool AckController::REQUIRE_ACKNOWLEDGMENT = true;

AckController::AckController (DisseminationService *pDisService, int64 i64TimeOut,
                              uint8 ui8TransmissionWindow)
    : MessagingService (pDisService), _cv (&_m)
{
    _i64TimeOut = i64TimeOut;
    _i64AckCyclePeriod = DEFAULT_RETRANSMISSION_CYCLE_PERIOD;
    _ui8TransmissionWindow = ui8TransmissionWindow;
    _pDisService = pDisService;
}

AckController::~AckController()
{
}

void AckController::run()
{
    const char *pszMethodName = "AckController::run";
    started();

    String *pTarget;
    String *pTmpString;
    MessageByTarget *pMBT;
    while (!terminationRequested()) {
        // For each client
        _m.lock();
        pTarget = _clientsToServe.peek();
        _m.unlock();
        if (pTarget == NULL) {
            // The queue is empty, wait some time...
            sleepForMilliseconds (1000);
        }
        else {
            // Send messages to the target
            pMBT = _messagesByTarget.get (pTarget->c_str());
            _mTopologyQueues.lock();
            if ((pTmpString = _newNeighbors.remove (pTarget)) != NULL) {
                delete pTmpString;
                pMBT->setAlive();
            }
            _mTopologyQueues.unlock();

            if (pMBT != NULL && pMBT->isAlive()) {
                replicateTo (pMBT, pTarget->c_str());
                _m.lock();
                _clientsToServe.dequeue();
                if (hasEnquequedMessages (pMBT)) {
                    // re-enqueue the target
                    _clientsToServe.enqueue (pTarget);
                }
                else {
                    // delete target
                    _messagesByTarget.remove (pTarget->c_str());
                    delete pMBT;
                    delete pTarget; 
                }
                _m.unlock();
            }
        }
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "Terminating replication.\n");
    terminating();
}

void AckController::replicateTo (MessageByTarget *pMBT, const char *pszTarget)
{
    if (pMBT == NULL || pszTarget == NULL) {
        return;
    }

    String target;
    String *pTmpString;
    while (pMBT->isAlive() && hasEnquequedMessages (pMBT)) {

        _m.lock();
        sendUnacknowledgedMessagesTo (pMBT, pszTarget);
        sendUnsentMessagesTo (pMBT, pszTarget);
        _mTopologyQueues.lock();
        target = pszTarget;
        if ((pTmpString = _deadNeighbors.remove (&target)) != NULL) {
            delete pTmpString;
            pMBT->setDead();
        }
        _mTopologyQueues.unlock();
        _cv.notifyAll();
        _m.unlock();

        sleepForMilliseconds (200);
    }
}

void AckController::sendUnacknowledgedMessagesTo (MessageByTarget *pMBT, const char *pszTarget)
{
    const char *pszMethodName = "AckController::sendUnacknowledgedMessagesTo";

    int64 i64Now = getTimeInMilliseconds();
    StringHashtable<MessageState>::Iterator states = pMBT->unacknowledgedMessages.getAllElements();
    for (; !states.end(); states.nextElement()) {
        MessageState *pMBS = states.getValue();
        if (pMBS != NULL && pMBS->hasTimedOut (i64Now)) {
            if (pMBS->bReplicationEnabled) {
                if (broadcastDataMessage (states.getKey(), pszTarget, _i64TimeOut,
                                          MessageInfo::DEFAULT_AVG_PRIORITY, REQUIRE_ACKNOWLEDGMENT,
                                          "Acknowledgment timed out. Resending message.") == 0) {
                    checkAndLogMsg (pszMethodName, Logger::L_Info,
                                    "the message <%s> is replicated\n", states.getKey());
                }
            }
            else {
                broadcastCtrlMessage (new DisServiceCompleteMessageReqMsg (_pDisService->getNodeId(), states.getKey()),
                                                                           "Sending DisServiceCompleteMessageReqMsg");
                checkAndLogMsg (pszMethodName, Logger::L_Info,
                                "sending complete message request for the msgId <%s>\n", states.getKey());
                if (((pMBS->ui8Counter+= 1) % 10) == 0) {
                    pMBS->ui8Counter = 0;
                    pMBS->bReplicationEnabled = true;
                }
            }
            pMBS->i64SendingTime = i64Now;
        }
    }
}

void AckController::sendUnsentMessagesTo (MessageByTarget *pMBT, const char *pszTarget)
{
    const char *pszMethodName = "AckController::sendQueuedMessagesTo";

    MessageHeader *pMHTmp;
    MessageHeader *pMH = pMBT->unsentMessages.getFirst();
    while ((pMH != NULL) && (pMBT->unacknowledgedMessages.getCount() < _ui8TransmissionWindow)) {
        // send the message
        if (broadcastDataMessage (pMH, pszTarget, _i64TimeOut, "Sending queued message") == 0) {
            // and add it to the unacknowledged messages list
            pMBT->unacknowledgedMessages.put (pMH->getMsgId(),
                                              new MessageState (getTimeInMilliseconds(), _i64TimeOut));
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "Transmission Window is %d, waiting for ack messages %d\n",
                            _ui8TransmissionWindow, pMBT->unacknowledgedMessages.getCount());
        }

        // remove the message from the queue of the unsent messages
        pMHTmp = pMBT->unsentMessages.getNext();
        pMH = pMBT->unsentMessages.remove (pMH);
        delete pMH;
        pMH = pMHTmp;
    }
}

void AckController::newIncomingMessage (const void *, uint16, DisServiceMsg *pDisServiceMsg, uint32, const char *)
{
    switch (pDisServiceMsg->getType()) {
        case DisServiceMsg::DSMT_DataReq: {
            DisServiceDataReqMsg *pReqMsg = (DisServiceDataReqMsg*) pDisServiceMsg;
            PtrLList<DisServiceDataReqMsg::FragmentRequest> *pRequests = pReqMsg->getRequests();
            DisServiceDataReqMsg::FragmentRequest *pRequest;
            DisServiceDataReqMsg::FragmentRequest *pRequestTmp = pRequests->getFirst();
            // Call messageRequested() for each requested message 
            while ((pRequest = pRequestTmp) != NULL) {
                pRequestTmp = pRequests->getNext();
                messageRequested (pRequest->pMsgHeader->getMsgId(),
                                  pDisServiceMsg->getSenderNodeId());
            }
            break;
        }

        case DisServiceMsg::DSMT_AcknowledgmentMessage: {
            DisServiceAcknowledgmentMessage *pAckMsg = (DisServiceAcknowledgmentMessage*) pDisServiceMsg;
            messageAcknowledged (pAckMsg->getAckedMsgId(), pDisServiceMsg->getSenderNodeId());
            break;
        }

        default:
            break;
    }
}

void AckController::newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                 const char *pszIncomingInterface)
{
    String *pNodeId = new String (pszNodeUID);
    _mTopologyQueues.lock();
    delete _deadNeighbors.remove (pNodeId);
    if (_newNeighbors.find (pNodeId) == NULL) {
        _newNeighbors.enqueue (pNodeId);
    }
    else {
        delete pNodeId;
    }
    _mTopologyQueues.unlock();
}

void AckController::deadNeighbor (const char *pszNodeUID)
{
    String *pNodeId = new String (pszNodeUID);
    _mTopologyQueues.lock();
    delete _newNeighbors.remove (pNodeId);
    if (_deadNeighbors.find (pNodeId) == NULL) {
        _deadNeighbors.enqueue (pNodeId);
    }
    else {
        delete pNodeId;
    }
    _mTopologyQueues.unlock();
}

void AckController::newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                       const char *pszIncomingInterface)
{
}

void AckController::droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr)
{    
}

void AckController::stateUpdateForPeer (const char *, PeerStateUpdateInterface *)
{
}

void AckController::ackRequest (const char *pszNodeUUID, MessageHeader *pMH)
{
    _m.lock();
    MessageByTarget *pMBT;
    pMBT = _messagesByTarget.get (pszNodeUUID);
    if (pMBT == NULL) {
        pMBT = new MessageByTarget();
        _messagesByTarget.put (pszNodeUUID, pMBT);
    }
    else {
        pMBT->setAlive();
    }

    if (pMH != NULL) {
        pMBT->unsentMessages.append (pMH->clone());
    }

    String *pNodeUUId = new String (pszNodeUUID);
    if (!_clientsToServe.find (pNodeUUId)) {
        _clientsToServe.enqueue (pNodeUUId); 
    }

    if (!this->isRunning()) {
        this->start();
    }

    _m.unlock();
}

void AckController::messageAcknowledged (const char *pszMessageAcknowledgedId, const char *pszSenderNodeId)
{
    _m.lock();
    MessageByTarget *pMBT = _messagesByTarget.get (pszSenderNodeId);
    if (pMBT == NULL) {
        _cv.notifyAll();
        _m.unlock();
        return;
    }

    MessageState *pMBS = pMBT->unacknowledgedMessages.remove (pszMessageAcknowledgedId);
    if (pMBS == NULL) {
        checkAndLogMsg ("Acknowledgment::messageAcknowledged", Logger::L_Info,
                        "Message <%s> for target node <%s> can't be deleted. Queue current size <%d>\n",
                        pszMessageAcknowledgedId, pszSenderNodeId,
                        pMBT->unacknowledgedMessages.getCount());
        _cv.notifyAll();
        _m.unlock();
        return;
    }

    checkAndLogMsg ("Acknowledgment::messageAcknowledged", Logger::L_Info,
                    "Message <%s> for target node <%s> eliminated from the waiting for "
                    "ack queue. Queue current size <%d>\n", pszMessageAcknowledgedId,
                    pszSenderNodeId, pMBT->unacknowledgedMessages.getCount());
    delete pMBS;

    _cv.notifyAll();
    _m.unlock();
}

void AckController::messageRequested (const char *pszMessageAcknowledgedId, const char *pszSenderNodeId)
{
    _m.lock();
    MessageByTarget *pMBT = _messagesByTarget.get (pszSenderNodeId);
    if (pMBT == NULL) {
        _cv.notifyAll();
        _m.unlock();
        return;
    }

    MessageState *pMBS = pMBT->unacknowledgedMessages.get (pszMessageAcknowledgedId);
    if (pMBS == NULL) {
        _cv.notifyAll();
        _m.unlock();
        return;
    }

    // If a request for a message is received, it means that the receiver is aware
    // of the existence of the message, thus, in compliance with Dissemination
    // Service design, the receiver is in charge to reassemble the message. thus
    // the replication for the message is disabled
    pMBS->bReplicationEnabled = false; 
    checkAndLogMsg ("Acknowledgment::dataRequestMsgAck", Logger::L_Info, "Message <%s> for "
                    "target node <%s> it will not be replicated. Queue current size <%d>\n",
                    pszMessageAcknowledgedId, pszSenderNodeId, pMBT->unacknowledgedMessages.getCount());

    _cv.notifyAll();
    _m.unlock();
}

bool AckController::hasEnquequedMessages (const char *pszSenderNodeId)
{
    return hasEnquequedMessages (_messagesByTarget.get (pszSenderNodeId));
}

bool AckController::hasEnquequedMessages (MessageByTarget *pMBT)
{
    if (pMBT == NULL) {
        return false;
    }
    return (pMBT->hasUnsentMessages() || pMBT->hasUnacknowledgedMessages());
}

bool AckController::MessageByTarget::hasUnsentMessages()
{
    return (unsentMessages.getFirst() != NULL);
}

bool AckController::MessageByTarget::hasUnacknowledgedMessages()
{
    return (unacknowledgedMessages.getCount() > 0);
}

