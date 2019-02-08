/*
 * SubscriptionState.cpp
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

#include "SubscriptionState.h"

#include "DisServiceDefs.h"
#include "DisseminationService.h"
#include "Message.h"
#include "MessageInfo.h"
#include "MessageReassembler.h"
#include "NodeInfo.h"

#include "Logger.h"
#include "SequentialArithmetic.h"
#include "MessageRequestScheduler.h"
#include "StringHashset.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

SubscriptionState::SubscriptionState(DisseminationService *pDisService, LocalNodeInfo *pLocalNodeInfo)
    : _states (true,    // bCaseSensitiveKeys
               true,    // bCloneKeys
               true,    // bDeleteKeys
               true),   // bDeleteValues
     _rcvdChunks(),
     _m (27)
{
    if (pDisService == NULL || pLocalNodeInfo == NULL) {
        checkAndLogMsg ("SubscriptionState::SubscriptionState", Logger::L_SevereError,
                        "SubscriptionState could not be initialized. Exiting.");
        exit (1);
    }

    _pDisService = pDisService;
    _pLocalNodeInfo = pLocalNodeInfo;
}

SubscriptionState::~SubscriptionState()
{
}

int SubscriptionState::messageArrived (Message *pMsg, RequestDetails *pDetails)
{
    return messageArrived (pMsg, false, pDetails);
}

int SubscriptionState::messageArrived (Message *pMsg, bool bIsInHistory, RequestDetails *pDetails)
{
    const char *pszMethodName = "SubscriptionState::messageArrivedInternal";

    if (pMsg->getMessageHeader()->isChunk()) {
        MessageHeader *pMH = pMsg->getMessageHeader();
        _m.lock (240);
        _rcvdChunks.put (pMH->getGroupName(), pMH->getPublisherNodeId(),
                         pMH->getMsgSeqId(), pMH->getChunkId());
        _m.unlock (240);
        notifyDisseminationServiceAndDeallocateMessage (pMsg, pDetails);
        return 0;
    }

    MessageInfo *pMI = pMsg->getMessageInfo();
    const String groupName (pMI->getGroupName());
    const String publisher (pMI->getPublisherNodeId());
    const uint32 ui32IncomingMsgSeqId = pMI->getMsgSeqId();
    const uint8 ui8IncomingMsgChunkId = pMI->getChunkId();

    DArray<uint16> *pSubscribingClients = _pLocalNodeInfo->getSubscribingClients (pMsg);
    _m.lock(241);
    if ((pSubscribingClients != NULL) && (isRelevantInternal (groupName, publisher, ui32IncomingMsgSeqId, ui8IncomingMsgChunkId) || bIsInHistory)) {

        //----------------------------------------------------------------------
        //  THERE IS AT LEAST ONE CLIENT SUBSCRIBING THE GROUP
        //----------------------------------------------------------------------
        const uint32 ui32CurrentExpectedMsgId = getSubscriptionStateInternal (groupName, publisher);
        const bool bSequential = _pLocalNodeInfo->requireSequentiality (groupName, pMI->getTag());
        const bool bReliable = _pLocalNodeInfo->requireGroupReliability (groupName, pMI->getTag());
        const bool bMetaDataWrappedInData = pMI->isMetaDataWrappedInData();

        //----------------------------------------------------------------------
        //  HISTORY MESSAGE
        //  (sequential/reliable transmission constraints do not not apply)
        //----------------------------------------------------------------------
        if (bIsInHistory) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "history message %s arrived.\n", pMI->getMsgId());
            _m.unlock (241);
            notifyDisseminationServiceAndDeallocateMessage (pMsg, pDetails);
            delete pSubscribingClients;
            pSubscribingClients = NULL;
            return 0;
        }

        //----------------------------------------------------------------------
        //  NON SEQUENTIAL COMMUNICATION
        //----------------------------------------------------------------------
        if (!bSequential) {
            notifyDisseminationService (pMsg, pDetails);
            if (!bMetaDataWrappedInData) {
                if (bReliable) {
                    nonSequentialReliableMessageArrived (groupName, publisher, ui32IncomingMsgSeqId);
                    if (ui32IncomingMsgSeqId == ui32CurrentExpectedMsgId) {
                        // Increment subscription state
                        // The transmission is reliable, the subscription state
                        // must be incremented only when the incoming message is
                        // the next expected.
                        setSubscriptionState (groupName, publisher, (ui32IncomingMsgSeqId + 1));
                    }
                }
                else {
                    nonSequentialUnreliableMessageArrived (groupName, publisher, ui32IncomingMsgSeqId);
                    if (ui32CurrentExpectedMsgId <= ui32IncomingMsgSeqId) {
                        // The transmission is not reliable, the subscription state
                        // must be incremented only when the incoming message is
                        // the next expected.
                        setSubscriptionState (groupName, publisher, (ui32IncomingMsgSeqId + 1));
                    }
                }
            }

            delete pSubscribingClients;
            pSubscribingClients = NULL;

            _m.unlock (241);
            deallocateMessage (pMsg);

            return 0;
        }

        //----------------------------------------------------------------------
        //  SEQUENTIAL COMMUNICATION
        //----------------------------------------------------------------------
        if (ui32IncomingMsgSeqId < ui32CurrentExpectedMsgId) {
            // The message is no longer relevant: drop it
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "received message %s, "
                            "that is not longer relevant; ui32CurrentExpectedMsgId = %u.\n",
                            pMI->getMsgId(), ui32CurrentExpectedMsgId);
            deallocateMessage (pMsg);
        }
        else if ((ui32IncomingMsgSeqId > ui32CurrentExpectedMsgId) && bReliable) {
            // I have to wait for the previous messages: buffer it!
            bufferMessage (pMsg);
        }
        else {
            // case ui32NewExpectedMsgId == ui32CurrentExpectedMsgId
            notifyDisseminationService (pMsg, pDetails);
            if (!bMetaDataWrappedInData) {
                setSubscriptionState (groupName, publisher, ui32IncomingMsgSeqId+1);
            }

            // Check also if there are buffered messages that are now allowed to
            // be passed to the client:
            if (bReliable && !bMetaDataWrappedInData) {
                bool bIsPreviousMetaDataWrappedInData = false;
                MessageInfo *pMITmp;
                for (Message *pMsgTmp = unbufferMessage (groupName, publisher, getSubscriptionStateInternal (groupName, publisher));
                     (pMsgTmp != NULL) && (!bIsPreviousMetaDataWrappedInData);
                     pMsgTmp = unbufferMessage (groupName, publisher, getSubscriptionStateInternal (groupName, publisher))) {

                    pMITmp = pMsgTmp->getMessageInfo();

                    notifyDisseminationService (pMsgTmp, pDetails);
                    if (!bMetaDataWrappedInData) {
                        setSubscriptionState (groupName, publisher, (pMsgTmp->getMessageHeader()->getMsgSeqId()+1));
                    }
                    bIsPreviousMetaDataWrappedInData = pMITmp->isMetaDataWrappedInData();
                    deallocateMessage (pMsgTmp);
                }
            }

            deallocateMessage (pMsg);
        }
    }
    else {
        deallocateMessage (pMsg);
    }
    _m.unlock (241);

    delete pSubscribingClients;
    pSubscribingClients = NULL;

    return 0;
}

bool SubscriptionState::containsMessage (const char *pszGroupName, const char *pszSenderNodeID, uint32 ui32SeqId)
{
    _m.lock (242);
    ByGroup *pByGroup = _states.get (pszGroupName);
    if (pByGroup == NULL) {
        _m.unlock (242);
        return false;
    }
    State *pState = pByGroup->_statesBySender.get (pszSenderNodeID);
    if (pState == NULL) {
        _m.unlock (242);
        return false;
    }
    if (pState->_ui8Type != SEQ_REL_STATE) {
        _m.unlock (242);
        return false;
    }
    SequentialReliableCommunicationState *pSeqState = (SequentialReliableCommunicationState *) pState;

    Message * pMsg = pSeqState->bufferedCompleteMessages.get (ui32SeqId);
    bool bRet = ((pMsg != NULL) && (!pMsg->getMessageInfo()->isMetaDataWrappedInData()));
    _m.unlock (242);
    return bRet;
}

void SubscriptionState::getMissingMessageRequests (MessageRequestScheduler *pReqScheduler, MessageReassembler *pMessageReassembler)
{
    _m.lock (243);
    ByGroup *pBG;
    const char *pszGroupName;
    for (StringHashtable<ByGroup>::Iterator iGroup = _states.getAllElements(); !iGroup.end(); iGroup.nextElement()) {
        pBG = iGroup.getValue();
        pszGroupName = iGroup.getKey();
        for (StringHashtable<State>::Iterator iSender = pBG->_statesBySender.getAllElements(); !iSender.end(); iSender.nextElement()) {
            State *pState = iSender.getValue();
            const String senderNodeId (iSender.getKey());
            /*!!*/ // This is a hack - prevent the sender from generating messages created at the local node

            if (senderNodeId != _pDisService->getNodeId()) {
                pState->fillUpMissingMessages (pReqScheduler, pMessageReassembler, pszGroupName,
                                               senderNodeId, (uint32)_pDisService->getMissingFragmentTimeout());
            }
        }
    }
    _m.unlock (243);
}

uint32 SubscriptionState::getSubscriptionState (const char *pszGroupName, const char *pszSenderNodeID)
{
    _m.lock (244);
    uint32 ui32NextExpectedSeqNo = getSubscriptionStateInternal (pszGroupName, pszSenderNodeID);
    _m.unlock (244);
    return ui32NextExpectedSeqNo;
}

uint32 SubscriptionState::getSubscriptionStateInternal (const char *pszGroupName, const char *pszSenderNodeID)
{
    ByGroup *pByGroup = _states.get(pszGroupName);
    if (pByGroup == NULL) {
         pByGroup = new ByGroup();
        _states.put (pszGroupName, pByGroup);
    }
    State *pState = pByGroup->_statesBySender.get(pszSenderNodeID);
    if (pState == NULL) {
        bool bSeq = _pLocalNodeInfo->requireSequentiality (pszGroupName, 0);
        bool bRel = _pLocalNodeInfo->requireGroupReliability (pszGroupName, 0);
        if (bSeq) {
            if (bRel) {
                pState = new SequentialReliableCommunicationState (this);
            }
            else {
                pState = new SequentialUnreliableCommunicationState (this);
            }
        }
        else {
            if (bRel) {
                pState = new NonSequentialReliableCommunicationState (this);
            }
            else {
                pState = new NonSequentialUnreliableCommunicationState (this);
            }
        }
        pByGroup->_statesBySender.put(pszSenderNodeID, pState);
    }
    return pState->ui32ExpectedSeqId;
}

bool SubscriptionState::isRelevant (const char *pszGroupName, const char *pszSenderNodeID,
                                    uint32 ui32IncomingMsgSeqId, uint8 ui8ChunkId)
{
    _m.lock(245);
    bool bRet = isRelevantInternal (pszGroupName, pszSenderNodeID, ui32IncomingMsgSeqId, ui8ChunkId);
    _m.unlock(245);
    return bRet;
}

bool SubscriptionState::isRelevantInternal (const char *pszGroupName, const char *pszSenderNodeID,
                                            uint32 ui32IncomingMsgSeqId, uint8 ui8ChunkId)
{
    if (ui8ChunkId != MessageHeader::UNDEFINED_CHUNK_ID) {
        // It's a chunk!
        return !_rcvdChunks.contains (pszGroupName, pszSenderNodeID,
                                      ui32IncomingMsgSeqId, ui8ChunkId);
    }

    ByGroup *pByGroup = _states.get (pszGroupName);
    if (pByGroup != NULL) {
        State *pState = pByGroup->_statesBySender.get (pszSenderNodeID);
        if ((pState != NULL) && (pState->bExpectedSeqIdSet)) {

            // If the bFirstSeqIdSet has not been set yet, it must return true
            return pState->isRelevant (ui32IncomingMsgSeqId);
        }
    }
    checkAndLogMsg ("WEIRD", Logger::L_Info, "%s:%s:u:%u.\n", pszGroupName, pszSenderNodeID,
                                                             ui32IncomingMsgSeqId, ui8ChunkId);
    return true;
}

void SubscriptionState::setSubscriptionState (const char *pszGroupName, const char *pszSenderNodeID, uint32 ui32NewExpectedSeqId)
{
    _m.lock (246);
    ByGroup *pByGroup = _states.get (pszGroupName);
    if (pByGroup == NULL) {
         pByGroup = new ByGroup();
        _states.put (pszGroupName, pByGroup);
    }
    State *pState = pByGroup->_statesBySender.get (pszSenderNodeID);
    if (pState == NULL) {
        bool bSeq, bRel;
        bSeq = _pLocalNodeInfo->requireSequentiality (pszGroupName, 0);
        bRel = _pLocalNodeInfo->requireGroupReliability (pszGroupName, 0);
        if (bSeq) {
            if (bRel) {
                pState = new SequentialReliableCommunicationState (this);
            }
            else {
                pState = new SequentialUnreliableCommunicationState (this);
            }
        }
        else {
            if (bRel) {
                pState = new NonSequentialReliableCommunicationState (this);
            }
            else {
                pState = new NonSequentialUnreliableCommunicationState (this);
            }
        }
        pByGroup->_statesBySender.put (pszSenderNodeID, pState);
    }

    if (!pState->bExpectedSeqIdSet) {
        pState->ui32ExpectedSeqId = ui32NewExpectedSeqId;
        pState->bExpectedSeqIdSet = true;
    }
    else if (SequentialArithmetic::greaterThan (ui32NewExpectedSeqId, pState->ui32ExpectedSeqId)) {
        if (pState->_ui8Type == NONSEQ_REL_STATE) {
            NonSequentialReliableCommunicationState *pNonSeqRel = (NonSequentialReliableCommunicationState*) pState;
            uint32 *pUI32FirstMissingMsg = pNonSeqRel->missingSeqId.getFirst();
            if (pUI32FirstMissingMsg) {
                pState->ui32ExpectedSeqId = ((*pUI32FirstMissingMsg > ui32NewExpectedSeqId) ? *pUI32FirstMissingMsg : ui32NewExpectedSeqId);
            }
            else if (pNonSeqRel->bHighestSeqIdRcvdSet && pNonSeqRel->ui32HighestSeqIdReceived == ui32NewExpectedSeqId) {
                pState->ui32ExpectedSeqId = ++ui32NewExpectedSeqId;
            }
            else {
                pState->ui32ExpectedSeqId = ui32NewExpectedSeqId;
            }
        }
        else {
            pState->ui32ExpectedSeqId = ui32NewExpectedSeqId;
        }
    }

    _m.unlock (246);
}

bool SubscriptionState::isSubscriptionBySenderEmpty (const char *pszGroupName, const char *pszSenderNodeId)
{
    _m.lock (247);
    ByGroup *pByGroup = _states.get(pszGroupName);
    if (pByGroup == NULL) {
        _m.unlock (247);
        return true;
    }

    bool bRet = (pByGroup->_statesBySender.get(pszSenderNodeId) == NULL);
    _m.unlock (247);
    return bRet;
}

uint32 SubscriptionState::getNumberOfMissingFragments (State* pState)
{
   if (pState->_ui8Type == SubscriptionState::SEQ_REL_STATE) {
       SequentialReliableCommunicationState *pStateSR = (SequentialReliableCommunicationState*) pState;
       return (pStateSR->ui32HighestSeqIdBuffered - pStateSR->ui32ExpectedSeqId -
               pStateSR->bufferedCompleteMessages.getCount());
   }
   else if (pState->_ui8Type == SubscriptionState::NONSEQ_REL_STATE) {
       NonSequentialReliableCommunicationState *pStateNSR = (NonSequentialReliableCommunicationState*) pState;
       int i = pStateNSR->missingSeqId.getCount();
       return (i < 0 ? 0 : (uint32)i);
   }

   return 0;
}

//////////////////////// SEQUENCED COMMUNICATION ///////////////////////////////

void SubscriptionState::bufferMessage (Message *pMsg)
{
    _m.lock (248);
    if (pMsg == NULL) {
        _m.unlock (248);
        return;
    }
    // The TransmissionServiceListener takes care to inizialize ByGroup and
    // State objects when the first fragment for the couple <senderNodeId, groupName>
    // is received.
    const char *pszGroupName = pMsg->getMessageInfo()->getGroupName();
    ByGroup *pByGroup = _states.get(pszGroupName);
    if (pByGroup == NULL) {
        _m.unlock (248);
        return;
    }
    const char *pszPublisherNodeID = pMsg->getMessageInfo()->getPublisherNodeId();
    State *pState = pByGroup->_statesBySender.get (pszPublisherNodeID);
    if (pState == NULL) {
        _m.unlock (248);
        return;
    }
    if (pState->_ui8Type != SEQ_REL_STATE) {
        _m.unlock (248);
        return;
    }
    SequentialReliableCommunicationState *pSeqState = (SequentialReliableCommunicationState *) pState;
    uint32 ui32SeqId = pMsg->getMessageInfo()->getMsgSeqId();
    pSeqState->i64LastNewMessageArrivalTime = getTimeInMilliseconds();
    Message *pOldMsg = pSeqState->bufferedCompleteMessages.get (ui32SeqId);
    if (pOldMsg != NULL && !pOldMsg->getMessageInfo()->isMetaDataWrappedInData()) {
        // TODO: I may have to delete something...
        _m.unlock (248);
        return;
    }

    pOldMsg = pSeqState->bufferedCompleteMessages.put (ui32SeqId, pMsg);
    if (pOldMsg != NULL) {
        delete (pOldMsg->getMessageInfo());
        free ((void *) pOldMsg->getData());
        delete pOldMsg;
        pOldMsg = NULL;
    }

    if ((pSeqState->ui32HighestSeqIdBuffered < ui32SeqId) && (!pMsg->getMessageInfo()->isMetaDataWrappedInData())) {
        pSeqState->ui32HighestSeqIdBuffered = ui32SeqId;
    }

    _m.unlock (248);
}

Message * SubscriptionState::unbufferMessage (const char *pszGroupName, const char *pszSenderNodeID, uint32 ui32SeqId)
{
    _m.lock (249);
    ByGroup *pByGroup = _states.get (pszGroupName);
    if (pByGroup == NULL) {
        _m.unlock (249);
        return NULL;
    }
    State *pState = pByGroup->_statesBySender.get (pszSenderNodeID);
    if (pState == NULL) {
        _m.unlock (249);
        return NULL;
    }
    if (pState->_ui8Type != SEQ_REL_STATE) {
        _m.unlock (249);
        return NULL;
    }
    SequentialReliableCommunicationState * pSeqState = (SequentialReliableCommunicationState *) pState;
    Message * pMsg = pSeqState->bufferedCompleteMessages.remove (ui32SeqId);

    _m.unlock (249);
    return pMsg;
}

void SubscriptionState::notifyDisseminationServiceAndDeallocateMessage (Message *pMessage, RequestDetails *pDetails)
{
    notifyDisseminationService (pMessage, pDetails);

    // NODE: in order to minimize the memcopies, the application is passed the
    // pointer to pData; this is not unsafe because pData is not used anymore,
    // and a copy of it has already been stored in the DataCache.
    // The MessageHeader object is not passed to the application, but its inner
    // fields are. Analogously to pData, a copy of MessageHeader has been stored
    // in the DataCache, so it can be deallocated. However, MessageInfo's inner
    // fields must not be deallocated, or alternatively the the application must
    // be given a copy
    deallocateMessage (pMessage);
}

void SubscriptionState::notifyDisseminationService (Message *pMessage, RequestDetails *pDetails)
{
    _pDisService->messageArrived (pMessage, pDetails);
}

void SubscriptionState::deallocateMessage (Message *pMessage)
{
    delete pMessage->getMessageHeader();
    free ((void*)pMessage->getData());
    delete pMessage;
    pMessage = NULL;
}

////////////// UNSEQUENCED UNRELIABLE TRANSMISSION /////////////////

void SubscriptionState::nonSequentialUnreliableMessageArrived (const char *pszGroupName, const char *pszSenderNodeID, uint32 ui32SeqId)
{
    _m.lock (250);
    if ((!pszGroupName) || (!pszSenderNodeID)) {
         _m.unlock (250);
        return;
    }
    ByGroup *pByGroup = _states.get(pszGroupName);
    if (pByGroup == NULL) {
        _m.unlock (250);
        return;
    }
    State *pState = pByGroup->_statesBySender.get(pszSenderNodeID);
    if (pState == NULL) {
        _m.unlock (250);
        return;
    }
    if (pState->_ui8Type != NONSEQ_UNREL_STATE) {
        _m.unlock (250);
        return;
    }
    NonSequentialUnreliableCommunicationState *pNonSeqUnrelState = (NonSequentialUnreliableCommunicationState *) pState;

    if (!pNonSeqUnrelState->receivedSeqId.hasTSN (ui32SeqId)) {
        pNonSeqUnrelState->receivedSeqId.addTSN (ui32SeqId);
    }
    pNonSeqUnrelState->i64LastNewMessageArrivalTime = getTimeInMilliseconds();
    _m.unlock (250);
}

////////////////////// UNSEQUENCED RELIABLE COMMUNICATION //////////////////////

void SubscriptionState::nonSequentialReliableMessageArrived (const char *pszGroupName, const char *pszSenderNodeID, uint32 ui32SeqId)
{
    _m.lock (251);
    ByGroup *pByGroup = _states.get (pszGroupName);
    if (pByGroup == NULL) {
        _m.unlock (252);
        return;
    }
    State *pState = pByGroup->_statesBySender.get (pszSenderNodeID);
    if (pState == NULL) {
        _m.unlock (253);
        return;
    }
    if (pState->_ui8Type != NONSEQ_REL_STATE) {
        _m.unlock (254);
        return;
    }
    NonSequentialReliableCommunicationState *pNonSeqState = (NonSequentialReliableCommunicationState *) pState;
    pNonSeqState->i64LastNewMessageArrivalTime = getTimeInMilliseconds();

    if (!pNonSeqState->bHighestSeqIdRcvdSet) {
        // FIRST MESSAGE COMPLETELY RECEIVED CASE:
        pNonSeqState->ui32HighestSeqIdReceived = ui32SeqId;
        pNonSeqState->bHighestSeqIdRcvdSet = true;
        if (SequentialArithmetic::greaterThan (ui32SeqId, pNonSeqState->ui32ExpectedSeqId)) {
            uint32 ui32FirstMissingSeqIdToAdd = pNonSeqState->ui32ExpectedSeqId;
            uint32 ui32LastMissingSeqIdToAdd = ui32SeqId - 1;
            while (SequentialArithmetic::lessThanOrEqual (ui32FirstMissingSeqIdToAdd, ui32LastMissingSeqIdToAdd)) {
                if (pNonSeqState->missingSeqId.search (&ui32FirstMissingSeqIdToAdd) == NULL) {
                    // If not present already, add it
                    uint32 *pUISeqId = new uint32[1];
                    if (pUISeqId != NULL) {
                        pUISeqId[0] = ui32FirstMissingSeqIdToAdd;
                        pNonSeqState->missingSeqId.insert (pUISeqId);
                    }
                }
                ui32FirstMissingSeqIdToAdd++;
            }
        }
        _m.unlock (254);
        return;
    }

    if (SequentialArithmetic::lessThan (ui32SeqId, pNonSeqState->ui32ExpectedSeqId)) {
        _m.unlock (254);
        return;
    }

    uint32 ui32HighestSeqIdTmp = pNonSeqState->ui32ExpectedSeqId - 1;
    if (SequentialArithmetic::lessThan (pNonSeqState->ui32HighestSeqIdReceived, ui32HighestSeqIdTmp)) {
        pNonSeqState->ui32HighestSeqIdReceived = ui32HighestSeqIdTmp;
    }

    uint32 ui32HighestSeqIdReceived = pNonSeqState->ui32HighestSeqIdReceived;
    uint32 ui32ExpectedSeqId = pNonSeqState->ui32ExpectedSeqId;

    uint32 ui32FirstMissingSeqIdToAdd;
    uint32 ui32LastMissingSeqIdToAdd;
    bool bHasToBeRemoved;

    if (SequentialArithmetic::greaterThan (ui32HighestSeqIdReceived, ui32ExpectedSeqId)) {
        if (SequentialArithmetic::greaterThan (ui32SeqId, ui32HighestSeqIdReceived)) {
            ui32FirstMissingSeqIdToAdd = ui32HighestSeqIdReceived + 1;
            ui32LastMissingSeqIdToAdd = ui32SeqId - 1;
            bHasToBeRemoved = false;
            pNonSeqState->ui32HighestSeqIdReceived = ui32SeqId;
        }
        else {
            // CASE: ui32NextExpectedSeqId <= ui32SeqId <= ui32HighestSeqIdReceived
            ui32FirstMissingSeqIdToAdd = 0; // No seq id needs to be added to
            ui32LastMissingSeqIdToAdd = -1; // the list of the missing messages
            bHasToBeRemoved = true;
        }
    }
    else {
        // CASE: ui32HighestSeqIdReceived <= ui32NextExpectedSeqId <= ui32SeqId
        ui32FirstMissingSeqIdToAdd = ui32ExpectedSeqId;
        ui32LastMissingSeqIdToAdd = ui32SeqId - 1;
        bHasToBeRemoved = true;
        pNonSeqState->ui32HighestSeqIdReceived = ui32SeqId;
    }

    uint32 *pUISeqId;
    // Add missing messages
    while (SequentialArithmetic::lessThanOrEqual (ui32FirstMissingSeqIdToAdd, ui32LastMissingSeqIdToAdd)) {
        if (pNonSeqState->missingSeqId.search (&ui32FirstMissingSeqIdToAdd) == NULL) {
            // If not present already, add it
            pUISeqId = new uint32[1];
            if (pUISeqId != NULL) {
                pUISeqId[0] = ui32FirstMissingSeqIdToAdd;
                pNonSeqState->missingSeqId.insert (pUISeqId);
            }
        }
        ui32FirstMissingSeqIdToAdd++;
    }
    // Remove the message received from the list of the missing messages
    if (bHasToBeRemoved) {
        pUISeqId = pNonSeqState->missingSeqId.remove (&ui32SeqId);
        if (pUISeqId != NULL) {
            delete[] pUISeqId;
            pUISeqId = NULL;
        }
        int *pCounter = pNonSeqState->requestCounters.get (ui32SeqId);
        if (pCounter) {
            delete pCounter;
            pCounter = NULL;
        }
    }

    _m.unlock (254);
}

//////////////////////////////// ByGroup ///////////////////////////////////////

SubscriptionState::ByGroup::ByGroup()
    : _statesBySender (true, // bCaseSensitiveKeys
                       true,  // bCloneKeys
                       true,  // bDeleteKeys
                       true)  // bDeleteValues
{
}

SubscriptionState::ByGroup::~ByGroup()
{
}

///////////////////////////////// State ////////////////////////////////////////

SubscriptionState::State::State(uint8 ui8Type, SubscriptionState *pParent)
{
    bExpectedSeqIdSet = false;
    ui32ExpectedSeqId = 0;
    i64LastNewMessageArrivalTime = 0;

    this->pParent = pParent;
    _ui8Type = ui8Type;
}

SubscriptionState::State::~State()
{
    pParent = NULL;
}

void SubscriptionState::State::fillUpMissingMessages (MessageRequestScheduler *pReqScheduler, MessageReassembler *pMessageReassembler,
                                                      const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MissingFragmentTimeout)
{
}

///////////////////////////// ReliableState ////////////////////////////////////

SubscriptionState::ReliableState::ReliableState (uint8 ui8Type, SubscriptionState *pParent)
    : State (ui8Type, pParent), requestCounters (US_INITSIZE,
                                                 true) // bDelValues
{
}

SubscriptionState::ReliableState::~ReliableState (void)
{
    UInt32Hashtable<int>::Iterator iter = requestCounters.getAllElements();
    int *pReqCounter;
    for (; !iter.end(); iter.nextElement()) {
        pReqCounter = requestCounters.remove (iter.getKey());
        delete pReqCounter;
    }
    pReqCounter = NULL;
}

//////////////////// NonSequentialUnreliableCommunicationState /////////////////

SubscriptionState::NonSequentialUnreliableCommunicationState::NonSequentialUnreliableCommunicationState(SubscriptionState *pParent)
    : State (NONSEQ_UNREL_STATE, pParent),
      receivedSeqId (true)
{
}

SubscriptionState::NonSequentialUnreliableCommunicationState::~NonSequentialUnreliableCommunicationState()
{
}

bool SubscriptionState::NonSequentialUnreliableCommunicationState::isRelevant (uint32 ui32IncomingMsgSeqId)
{
    /*if (!bExpectedSeqIdSet) {
        // It means that no message has been received yet for the subscription,
        // therefore it is always relevant
        return true;
    }*/
    return !receivedSeqId.hasTSN (ui32IncomingMsgSeqId);
}

////////////////////// SequentialReliableCommunicationState ////////////////////

SubscriptionState::SequentialReliableCommunicationState::SequentialReliableCommunicationState(SubscriptionState *pParent)
    : ReliableState (SEQ_REL_STATE, pParent), bufferedCompleteMessages (US_INITSIZE, true)
{
    ui32HighestSeqIdBuffered = 0;
    i64LastMissingMessageRequestTime = 0;
}

SubscriptionState::SequentialReliableCommunicationState::~SequentialReliableCommunicationState()
{
    UInt32Hashtable<Message>::Iterator iter = bufferedCompleteMessages.getAllElements();
    Message *pMsg;
    for (; !iter.end(); iter.nextElement()) {
        pMsg = bufferedCompleteMessages.remove (iter.getKey());
        delete (pMsg->getMessageHeader());
        free ((void *) pMsg->getData());
        delete pMsg;
    }
    pMsg = NULL;
}

void SubscriptionState::SequentialReliableCommunicationState::fillUpMissingMessages (MessageRequestScheduler *pReqScheduler,
                                                                                     MessageReassembler *pMessageReassembler,
                                                                                     const char *pszGroupName, const char *pszSenderNodeId,
                                                                                     uint32 ui32MissingFragmentTimeout)
{
    int64 i64CurrentTime = getTimeInMilliseconds();
    if (pMessageReassembler->usingExponentialBackOff()) {
        // Exponential Backoff
        int64 i64ElapsedTimeSinceLastNewMessage = i64CurrentTime - i64LastNewMessageArrivalTime;

        uint32 ui32ElapsedTimeFactor = (uint32) (i64ElapsedTimeSinceLastNewMessage / ui32MissingFragmentTimeout);
        int64 i64TimeOut = 0;
        if (ui32ElapsedTimeFactor < 3) {
            i64TimeOut = ui32MissingFragmentTimeout;
        }
        else {
            ui32ElapsedTimeFactor -= 3;
            i64TimeOut = ui32ElapsedTimeFactor * ui32ElapsedTimeFactor  * ui32MissingFragmentTimeout;    // Exponential backoff
        }
        if (i64CurrentTime < (i64LastMissingMessageRequestTime + i64TimeOut)) {
            // Too soon to request missing messages for this group - skip it for now
            return;
        }
    }

    int *pCounter = NULL;
    int iReqLimit = pMessageReassembler->getRequestLimit();

    for (uint32 ui32MsgSeqId = ui32ExpectedSeqId; ui32MsgSeqId < ui32HighestSeqIdBuffered; ui32MsgSeqId++) {
        pCounter = requestCounters.get (ui32MsgSeqId);
        if ((iReqLimit != MessageReassembler::UNLIMITED_MAX_NUMBER_OF_REQUESTS) &&
            (pCounter != NULL)) {
            if ((*pCounter) > iReqLimit) {
                checkAndLogMsg ("SequentialReliableCommunicationState::fillUpMissingMessages", Logger::L_Info,
                                "Missing message %s:%s:%u has been requested too many times.",
                                pszGroupName, pszSenderNodeId, ui32MsgSeqId);
                // This message has been requested enough times
                continue;
            }
        }

        if ((bufferedCompleteMessages.get (ui32MsgSeqId) == NULL) &&
            (!pMessageReassembler->containsMessage (pszGroupName, pszSenderNodeId, ui32MsgSeqId))) {
            // Add the request
            pReqScheduler->addRequest (new DisServiceDataReqMsg::MessageRequest (pszGroupName, pszSenderNodeId, ui32MsgSeqId), true, true);
            if (pCounter == NULL) {
                pCounter = new int[1];
                if (pCounter != NULL) {
                    (*pCounter) = 1;
                    requestCounters.put (ui32MsgSeqId, pCounter);
                }
            }
            else {
                // MAY have the same problem of wrong updates of pCounter as the
                // not sequential reliable version. but probably the sequentiality
                // helps here. anyways, check that one for details
                (*pCounter) += 1;
            }

        }
    }
    i64LastMissingMessageRequestTime = i64CurrentTime;
}

bool SubscriptionState::SequentialReliableCommunicationState::isRelevant (uint32 ui32IncomingMsgSeqId)
{
    if (!bExpectedSeqIdSet) {
        // It means that no message has been received yet for the subscription,
        // therefore it is always relevant
        return true;
    }

    if (ui32IncomingMsgSeqId >= ui32ExpectedSeqId) {
        Message * pMsg = bufferedCompleteMessages.get(ui32IncomingMsgSeqId);
        if ((pMsg != NULL) && (!pMsg->getMessageInfo()->isMetaDataWrappedInData())) {
            return false;
        }
        return true;
    }
    return false;
}

void SubscriptionState::SequentialReliableCommunicationState::skipMessage (uint32 ui32MsgSeqId)
{
    Message *pMsg = bufferedCompleteMessages.get (ui32MsgSeqId);
    if (pMsg != NULL) {
        checkAndLogMsg ("SubscriptionState::SequentialReliableCommunicationState::skipMessage",
                        Logger::L_Warning, "Trying to skip a received message\n");
        // I received it, I can not skip it...
        return;
    }
    if (ui32MsgSeqId == ui32ExpectedSeqId) {
        // Update the next expected seq id
        ui32ExpectedSeqId++;
        bool bIsPreviousMetaDataWrappedInData = false;
        MessageInfo *pMI = NULL;
        // See if there's some buffered message that was in hold waiting for
        // ui32MsgSeqId to arrive:
        for (uint32 ui32 = ui32ExpectedSeqId; (pMsg = bufferedCompleteMessages.get(ui32)) != NULL; ui32++) {
            if (bIsPreviousMetaDataWrappedInData) {
                break;
            }

            // Deliver message
            // TODO: pass the proper value of pszQueryId!!!
            pParent->notifyDisseminationService (pMsg, NULL);

            pMI = pMsg->getMessageInfo();
            if (pMI == NULL) {
                break;
            }
            bIsPreviousMetaDataWrappedInData = pMI->isMetaDataWrappedInData();
            if (!bIsPreviousMetaDataWrappedInData) {
                ui32ExpectedSeqId = ui32 + 1;
            }

            pParent->deallocateMessage (pMsg);
        }
    }
}

/////////////////// SequentialUnreliableCommunicationState /////////////////////

SubscriptionState::SequentialUnreliableCommunicationState::SequentialUnreliableCommunicationState (SubscriptionState *pParent)
    : State (SEQ_UNREL, pParent)
{
}

SubscriptionState::SequentialUnreliableCommunicationState::~SequentialUnreliableCommunicationState (void)
{
}

bool SubscriptionState::SequentialUnreliableCommunicationState::isRelevant (uint32 ui32IncomingMsgSeqId)
{
    if (!bExpectedSeqIdSet) {
        // It means that no message has been received yet for the subscription,
        // therefore it is always relevant
        return true;
    }
    if (ui32IncomingMsgSeqId >= ui32ExpectedSeqId) {
        return true;
    }
    return false;
}

/////////////////// NonSequentialReliableCommunicationState ////////////////////

SubscriptionState::NonSequentialReliableCommunicationState::NonSequentialReliableCommunicationState (SubscriptionState *pParent)
    : ReliableState (NONSEQ_REL_STATE, pParent), missingSeqId (false)  // descendingOrder
{
    ui32HighestSeqIdReceived = 0;
    bHighestSeqIdRcvdSet = false;
    i64LastMissingMessageRequestTime = 0;
}

SubscriptionState::NonSequentialReliableCommunicationState::~NonSequentialReliableCommunicationState()
{
    uint32 *pSeqId, *pSeqIdTmp;
    pSeqIdTmp = missingSeqId.getFirst();
    while ((pSeqId = pSeqIdTmp) != NULL) {
        pSeqIdTmp = missingSeqId.getNext();
        missingSeqId.remove (pSeqId);
        delete pSeqId;
    }
}

void SubscriptionState::NonSequentialReliableCommunicationState::fillUpMissingMessages (MessageRequestScheduler *pReqScheduler,
                                                                                        MessageReassembler *pMessageReassembler,
                                                                                        const char *pszGroupName, const char *pszSenderNodeId,
                                                                                        uint32 ui32MissingFragmentTimeout)
{
    int64 i64CurrentTime = getTimeInMilliseconds();
    if (pMessageReassembler->usingExponentialBackOff()) {
        // Exponential Backoff
        int64 i64ElapsedTimeSinceLastNewMessage = i64CurrentTime - i64LastNewMessageArrivalTime;
        uint32 ui32ElapsedTimeFactor = (uint32) (i64ElapsedTimeSinceLastNewMessage / ui32MissingFragmentTimeout);
        int64 i64TimeOut = 0;
        if (ui32ElapsedTimeFactor < 3) {
            i64TimeOut = ui32MissingFragmentTimeout;
        }
        else {
            ui32ElapsedTimeFactor -= 3;
            i64TimeOut = ui32ElapsedTimeFactor * ui32ElapsedTimeFactor  * ui32MissingFragmentTimeout;    // Exponential backoff
        }
        if (i64CurrentTime < (i64LastMissingMessageRequestTime + i64TimeOut)) {
            // Too soon to request missing messages for this group - skip it for now
            return;
        }
    }

    int *pCounter = NULL;
    int iReqLimit = pMessageReassembler->getRequestLimit();

    for (uint32 *pUI32SeqId = missingSeqId.getFirst(); pUI32SeqId != NULL; pUI32SeqId = missingSeqId.getNext()) {
        pCounter = requestCounters.get (*pUI32SeqId);

        if ((iReqLimit != MessageReassembler::UNLIMITED_MAX_NUMBER_OF_REQUESTS) &&
            (pCounter != NULL)) {
            if ((*pCounter) > iReqLimit) {
                checkAndLogMsg ("NonSequentialReliableCommunicationState::fillUpMissingMessages", Logger::L_Info,
                                "Missing message %s:%s:%u has been requested too many times.",
                                pszGroupName, pszSenderNodeId, (*pUI32SeqId));
                continue;
            }
        }

        if (!pMessageReassembler->containsMessage (pszGroupName, pszSenderNodeId, (*pUI32SeqId))) {
            // create and add the request
            pReqScheduler->addRequest (new DisServiceDataReqMsg::MessageRequest (pszGroupName, pszSenderNodeId, (*pUI32SeqId)), false, true);
            if (pCounter == NULL) {
                pCounter = new int[1];
            }
            /*
             * the counter should be increased every time the message is requested, but we update it here without knowing
             * if it'll be really requested or not (because we ask only for a limited number of missing messages, not all)
             * for now, we'll simply keep it at 1, without changing the value
             */
            if (pCounter != NULL) {
                (*pCounter) = 1;
                requestCounters.put ((*pUI32SeqId), pCounter);
            }
        }
    }
    i64LastMissingMessageRequestTime = i64CurrentTime;
}

bool SubscriptionState::NonSequentialReliableCommunicationState::isRelevant (uint32 ui32IncomingMsgSeqId)
{
    if (!bExpectedSeqIdSet) {
        // It means that no message has been received yet for the subscription,
        // therefore it is always relevant
        checkAndLogMsg ("WEIRD", Logger::L_Info, "bExpectedSeqIdSet = false :u:.\n", ui32IncomingMsgSeqId);
        return true;
    }
    if (ui32IncomingMsgSeqId >= ui32ExpectedSeqId) {
        if (bHighestSeqIdRcvdSet) {
            if (ui32IncomingMsgSeqId < ui32HighestSeqIdReceived) {
                if (missingSeqId.search (&ui32IncomingMsgSeqId) == NULL) {
                    // The message is NOT missing
                    return false;
                }
            }
            else if (ui32IncomingMsgSeqId == ui32HighestSeqIdReceived) {
                return false;
            }
        }
        return true;
    }
    return false;
}

//==============================================================================
// ReceivedChunks
//==============================================================================

SubscriptionState::ReceivedChunks::ReceivedChunks()
    : _byGroupName (true, true, true, true)
{
}

SubscriptionState::ReceivedChunks::~ReceivedChunks()
{
}

int SubscriptionState::ReceivedChunks::put (const char *pszGroupName,
                                            const char *pszSenderNodeId,
                                            uint32 ui32MsgSeqId, uint8 ui8ChunkId)
{
    BySender *pBySender = _byGroupName.get (pszGroupName);
    if (pBySender == NULL) {
        pBySender = new BySender (true, true, true, true);
        if (pBySender == NULL) {
            checkAndLogMsg ("ReceivedChunks::put", memoryExhausted);
            return -1;
        }
        _byGroupName.put (pszGroupName, pBySender);
    }

    BySeqId *pBySeqId = pBySender->get (pszSenderNodeId);
    if (pBySeqId == NULL) {
        pBySeqId = new BySeqId (true);
        if (pBySeqId == NULL) {
            checkAndLogMsg ("ReceivedChunks::put", memoryExhausted);
            return -2;
        }
        pBySender->put (pszSenderNodeId, pBySeqId);
    }

    ChunkIdList *pChunkList = pBySeqId->get (ui32MsgSeqId);
    if (pChunkList == NULL) {
        pChunkList = new ChunkIdList (false);
        if (pBySeqId == NULL) {
            checkAndLogMsg ("ReceivedChunks::put", memoryExhausted);
            return -3;
        }
        pBySeqId->put (ui32MsgSeqId, pChunkList);
    }

    return pChunkList->addTSN (ui8ChunkId);
}

bool SubscriptionState::ReceivedChunks::contains (const char *pszGroupName,
                                                  const char *pszSenderNodeId,
                                                  uint32 ui32MsgSeqId,
                                                  uint8 ui8ChunkId)
{
    BySender *pBySender = _byGroupName.get (pszGroupName);
    if (pBySender == NULL) {
        return false;
    }

    BySeqId *pBySeqId = pBySender->get (pszSenderNodeId);
    if (pBySeqId == NULL) {
        return false;
    }

    ChunkIdList *pChunkList = pBySeqId->get (ui32MsgSeqId);
    if (pChunkList == NULL) {
        return false;
    }

    return pChunkList->hasTSN (ui8ChunkId);
}

