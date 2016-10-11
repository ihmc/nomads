/*
 * SubscriptionStateCapped.cpp
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

#include "SubscriptionStateCapped.h"

#include "DisServiceDefs.h"
#include "DisseminationService.h"
#include "Message.h"
#include "MessageInfo.h"
#include "MessageReassembler.h"
#include "NodeInfo.h"

#include "Logger.h"
#include "SequentialArithmetic.h"
#include "MessageRequestScheduler.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

SubscriptionStateCapped::SubscriptionStateCapped (DisseminationService *pDisService, LocalNodeInfo *pLocalNodeInfo)
    : SubscriptionState (pDisService, pLocalNodeInfo)
{
    _MaxNumberOfMissingMessagesToFillUp=DEFAULT_MAX_MISSING_MESSAGES_TO_FILL_UP;
    checkAndLogMsg ("SubscriptionStateCapped::SubscriptionStateCap", Logger::L_Info,
                    "Maximum number of missing messages considered: %u\n",
                    _MaxNumberOfMissingMessagesToFillUp);

}

SubscriptionStateCapped::~SubscriptionStateCapped()
{
}

void SubscriptionStateCapped::setMaxMissingMessagesToFillUp (uint32 ui32Max)
{
    _MaxNumberOfMissingMessagesToFillUp = ui32Max;
    checkAndLogMsg ("SubscriptionStateCapped::SubscriptionStateCap", Logger::L_Info,
                    "Maximum number of missing messages considered changed to: %u\n",
                    _MaxNumberOfMissingMessagesToFillUp);
}

uint32 SubscriptionStateCapped::getFillUpAmount (const char *pszGroupNameWanted, const char *pszSenderNodeIdWanted)
{
    /*
     * horrible implementation, can be improved. possible ways:
     * - calculate quotas only 1 time, keep them in a list field of SubscriptionStateCapped.
     *   Just go through the list to return the value
     * - try to see if we can reduce a bit the cycles below here
     */

    NOMADSUtil::StringHashtable<ByGroup> _statesCopy;
    ByGroup * pBG;
    const char * pszGroupName;
    uint32 ui32WeightedTotal = 0;

    //TOTAL AND MAKE COPY
    for (StringHashtable<ByGroup>::Iterator iGroup = _states.getAllElements(); !iGroup.end(); iGroup.nextElement()) {
        pBG = iGroup.getValue();
        pszGroupName = iGroup.getKey();

        for (StringHashtable<State>::Iterator iSender = pBG->_statesBySender.getAllElements(); !iSender.end(); iSender.nextElement()) {
            State *pState = iSender.getValue();
            const char * pszSenderNodeId = iSender.getKey();
            //update the weighted total
            uint32 ui32MissingFragments = SubscriptionState::getNumberOfMissingFragments(pState);
            double dWeightedMissingFragments = ui32MissingFragments * getStateTypeBasedPriorityFactor (pState);
            ui32WeightedTotal = ui32WeightedTotal +  ((uint32) ceil (dWeightedMissingFragments));
            ByGroup *pByGroup = _statesCopy.get (pszGroupName);
            if (pByGroup == NULL) {
                 pByGroup = new ByGroup();
                _statesCopy.put (pszGroupName, pByGroup);
            }
            pByGroup->_statesBySender.put (pszSenderNodeId,pState);

        }
    }

    //PRELIMINARY RESULTS, SEE IF THERE ARE STATES WIH LESS FRAGMENTS THAN THEIR QUOTA. PUT THEM IN FINAL LIST AND RECALCULATE

    bool bRecalculate = true;
    bool bResultCalculated = false;
    uint32 ui32Result = 0;
    uint32 ui32SpaceAvailableToFillUp = _MaxNumberOfMissingMessagesToFillUp;

    while (bRecalculate){
        bRecalculate=false;
        for (StringHashtable<ByGroup>::Iterator iGroup = _statesCopy.getAllElements(); (!iGroup.end() && !bResultCalculated); iGroup.nextElement()) {
            pBG = iGroup.getValue();
            pszGroupName = iGroup.getKey();
            for (StringHashtable<State>::Iterator iSender = pBG->_statesBySender.getAllElements(); (!iSender.end() && !bResultCalculated); iSender.nextElement()) {
                State *pState = iSender.getValue();
                const char * pszSenderNodeId = iSender.getKey();
                uint32 ui32MissingFragments, ui32FragmentsToAsk;

                ui32MissingFragments = getNumberOfMissingFragments(pState);
                double dFragmentsToAsk = ui32MissingFragments * getStateTypeBasedPriorityFactor (pState) * ui32SpaceAvailableToFillUp / ui32WeightedTotal;
                ui32FragmentsToAsk = (uint32) ceil(dFragmentsToAsk);

                if (ui32FragmentsToAsk > ui32MissingFragments){
                    if((!strcmp (pszSenderNodeId,pszSenderNodeIdWanted))&&(!strcmp (pszGroupName,pszGroupNameWanted))){
                        bResultCalculated =  true;
                        ui32Result = ui32MissingFragments;
                    }
                    ui32WeightedTotal = ui32WeightedTotal - ((uint32) ceil (ui32MissingFragments * getStateTypeBasedPriorityFactor (pState)));
                    ui32SpaceAvailableToFillUp -= ui32MissingFragments;
                    pBG->_statesBySender.remove (pszSenderNodeId);//remove the node from the table, we don't need to consider it anymore (at least in this function call)
                    bRecalculate = true;
                }
            }
            //if we already removed all the elements of the group, remove the group
            if(pBG){
                if(pBG->_statesBySender.getCount()==0){
                    _statesCopy.remove (pszGroupName);
                    delete pBG;
                }
            }
        }
    }

    //ADD THE REMAINING STATES TO FINAL LIST
    for (StringHashtable<ByGroup>::Iterator iGroup = _statesCopy.getAllElements(); (!iGroup.end() && !bResultCalculated); iGroup.nextElement()) {
        pBG = iGroup.getValue();
        pszGroupName = iGroup.getKey();
        for (StringHashtable<State>::Iterator iSender = pBG->_statesBySender.getAllElements(); (!iSender.end() && !bResultCalculated); iSender.nextElement()) {
            State *pState = iSender.getValue();
            const char * pszSenderNodeId = iSender.getKey();
            uint32 ui32MissingFragments, ui32FragmentsToAsk;

            ui32MissingFragments = getNumberOfMissingFragments (pState);
            double dFragmentsToAsk = ui32MissingFragments * getStateTypeBasedPriorityFactor (pState) * ui32SpaceAvailableToFillUp / ui32WeightedTotal;
            ui32FragmentsToAsk = (uint32) ceil (dFragmentsToAsk);
            checkAndLogMsg ("SubscriptionStateCapped::getFillUpAmount", Logger::L_LowDetailDebug,
                            "subscription %s-%s misses %u fragments and its quota is %u\n",
                            pszGroupName,pszSenderNodeId,ui32MissingFragments,ui32FragmentsToAsk);
            if((!strcmp (pszSenderNodeId,pszSenderNodeIdWanted))&&(!strcmp (pszGroupName,pszGroupNameWanted))){
                bResultCalculated =  true;
                ui32Result = ui32FragmentsToAsk;
            }

            pBG->_statesBySender.remove (pszSenderNodeId);//remove the node from the table, we don't need to consider it anymore (at least in this function call)
        }
        if(pBG){
            _statesCopy.remove (pszGroupName);
            delete pBG;
        }
    }
    _statesCopy.removeAll();

    return ui32Result;
}
/*
SubscriptionStateCapped::CapsByGroup::CapsByGroup()
{

}

SubscriptionStateCapped::CapsByGroup::~CapsByGroup()
{
   for (StringHashtable<uint32>::Iterator iSender = this->_capsBySender.getAllElements(); !iSender.end(); iSender.nextElement()) {
       uint32* pInt = iSender.getValue();
       delete pInt;
   }
}
*/
SubscriptionStateCapped::SequentialReliableCommunicationStateCapped::SequentialReliableCommunicationStateCapped (SubscriptionState *pParent)
    : SequentialReliableCommunicationState (pParent)
{

}

SubscriptionStateCapped::SequentialReliableCommunicationStateCapped::~SequentialReliableCommunicationStateCapped()
{

}

void SubscriptionStateCapped::SequentialReliableCommunicationStateCapped::fillUpMissingMessages (MessageRequestScheduler *pReqScheduler,
                                                                                                 MessageReassembler *pMessageReassembler,
                                                                                                 const char *pszGroupName,
                                                                                                 const char *pszSenderNodeId,
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
    int * pCounter = NULL;
    int iReqLimit = pMessageReassembler->getRequestLimit();
    uint32 ui32AddedRequests = 0;
    uint32 ui32RequestsToAdd;
    ui32RequestsToAdd = ((SubscriptionStateCapped *) this->pParent)->getFillUpAmount (pszGroupName, pszSenderNodeId);

    for (uint32 ui32MsgSeqId = ui32ExpectedSeqId; (ui32MsgSeqId < ui32HighestSeqIdBuffered) && (ui32AddedRequests < ui32RequestsToAdd); ui32MsgSeqId++) {
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
            pReqScheduler->addRequest(new DisServiceDataReqMsg::MessageRequest (pszGroupName, pszSenderNodeId, ui32MsgSeqId), true, true);
            ui32AddedRequests++;
            if (pCounter == NULL) {
                pCounter = new int[1];
                if (pCounter != NULL) {
                    (*pCounter) = 1;
                    requestCounters.put (ui32MsgSeqId, pCounter);
                }
            }
            else {
                // MAY have the same problem of wrong updates of pCounter as the not sequential reliable version.
                // but probably the sequentiality helps here. anyways, check that one for details
                (*pCounter) += 1;
            }

        }
    }

    i64LastMissingMessageRequestTime = i64CurrentTime;
}



SubscriptionStateCapped::NonSequentialReliableCommunicationStateCapped::NonSequentialReliableCommunicationStateCapped (SubscriptionState *pParent)
    : NonSequentialReliableCommunicationState(pParent)
{

}

SubscriptionStateCapped::NonSequentialReliableCommunicationStateCapped::~NonSequentialReliableCommunicationStateCapped()
{

}

void SubscriptionStateCapped::NonSequentialReliableCommunicationStateCapped::fillUpMissingMessages (MessageRequestScheduler *pReqScheduler,
                                                                                                    MessageReassembler *pMessageReassembler,
                                                                                                    const char *pszGroupName,
                                                                                                    const char *pszSenderNodeId,
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

    int * pCounter = NULL;
    int iReqLimit = pMessageReassembler->getRequestLimit();
    uint32 ui32AddedRequests = 0;
    uint32 ui32RequestsToAdd;
    ui32RequestsToAdd = ((SubscriptionStateCapped *) this->pParent)->getFillUpAmount (pszGroupName, pszSenderNodeId);

    int contatore=0;

    for (uint32 * pUI32SeqId = missingSeqId.getFirst(); (pUI32SeqId != NULL) && (ui32AddedRequests < ui32RequestsToAdd); pUI32SeqId = missingSeqId.getNext()) {
        pCounter = requestCounters.get (*pUI32SeqId);

        contatore++;

        if ((iReqLimit != MessageReassembler::UNLIMITED_MAX_NUMBER_OF_REQUESTS) &&
            (pCounter != NULL)) {
            if ((*pCounter) > iReqLimit) {
                checkAndLogMsg ("NonSequentialReliableCommunicationState::fillUpMissingMessages", Logger::L_LowDetailDebug,
                                "Missing message %s:%s:%u has been requested too many times.",
                                pszGroupName, pszSenderNodeId, (*pUI32SeqId));
                continue;
            }
        }

        if (!pMessageReassembler->containsMessage(pszGroupName, pszSenderNodeId, (*pUI32SeqId))) {
            // create and add the request
            pReqScheduler->addRequest (new DisServiceDataReqMsg::MessageRequest (pszGroupName, pszSenderNodeId, (*pUI32SeqId)), false, true);
            ui32AddedRequests++;
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

    checkAndLogMsg ("NonSequentialReliableCommunicationState::fillUpMissingMessages", Logger::L_LowDetailDebug,
                                        "Missing messages for %s:%s = %u. Cycled missing messages = %u\n",
                                        pszGroupName, pszSenderNodeId, missingSeqId.getCount(),contatore);
}

void SubscriptionStateCapped::setSubscriptionState (const char *pszGroupName, const char *pszSenderNodeID, uint32 ui32NewExpectedSeqId)
{
    _m.lock (255);
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
                pState = new SequentialReliableCommunicationStateCapped (this);
            }
            else {
                pState = new SequentialUnreliableCommunicationState (this);
            }
        }
        else {
            if (bRel) {
                pState = new NonSequentialReliableCommunicationStateCapped (this);
            }
            else {
                pState = new NonSequentialUnreliableCommunicationState (this);
            }
        }
        pByGroup->_statesBySender.put(pszSenderNodeID, pState);
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
            else if ( pNonSeqRel->bHighestSeqIdRcvdSet && pNonSeqRel->ui32HighestSeqIdReceived == ui32NewExpectedSeqId) {
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

    _m.unlock (255);
}

uint32 SubscriptionStateCapped::getSubscriptionState (const char *pszGroupName, const char *pszSenderNodeID)
{
    _m.lock (256);
    ByGroup *pByGroup = _states.get (pszGroupName);
    if (pByGroup == NULL) {
         pByGroup = new ByGroup();
        _states.put (pszGroupName, pByGroup);
    }
    State *pState = pByGroup->_statesBySender.get(pszSenderNodeID);
    if (pState == NULL) {
        bool bSeq= _pLocalNodeInfo->requireSequentiality (pszGroupName, 0);
        bool bRel = _pLocalNodeInfo->requireGroupReliability (pszGroupName, 0);
        if (bSeq) {
            if (bRel) {
                pState = new SequentialReliableCommunicationStateCapped (this);
            }
            else {
                pState = new SequentialUnreliableCommunicationState (this);
            }
        }
        else {
            if (bRel) {
                pState = new NonSequentialReliableCommunicationStateCapped (this);
            }
            else {
                pState = new NonSequentialUnreliableCommunicationState (this);
            }
        }
        pByGroup->_statesBySender.put (pszSenderNodeID, pState);
    }
    _m.unlock (256);
    return pState->ui32ExpectedSeqId;
}

float SubscriptionStateCapped::getStateTypeBasedPriorityFactor (State* pState )
{
    //for now, simply give higher priority to seq rel subscriptions, less to non seq rel, 0 to others
    if (pState->_ui8Type == SubscriptionState::SEQ_REL_STATE) {
        return 1.1f;
    }
    else if (pState->_ui8Type == SubscriptionState::NONSEQ_REL_STATE) {
        return 0.9f;
    }
    else {
        return 0.0f;
    }
}
