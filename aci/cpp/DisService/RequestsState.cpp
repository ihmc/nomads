/*
 * RequestsState.cpp
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

#include "RequestsState.h"

#include "DisServiceDefs.h"
#include "Message.h"
#include "MessageInfo.h"
#include "MessageReassembler.h"
#include "MessageRequestScheduler.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//------------------------------------------------------------------------------
// RequestsInterface
//------------------------------------------------------------------------------

RequestsInterface::RequestsInterface()
{   
}

RequestsInterface::~RequestsInterface()
{
}

//------------------------------------------------------------------------------
// AbstractRequestsState
//------------------------------------------------------------------------------

AbstractRequestsState::AbstractRequestsState()
    : _states (true,  // bCaseSensitiveKeys
               true,  // bCloneKeys
               true,  // bDeleteKeys
               true)  // bDeleteValues
{
}

AbstractRequestsState::~AbstractRequestsState()
{
}

//------------------------------------------------------------------------------
// RequestsState
//------------------------------------------------------------------------------

MessageRequestState::MessageRequestState()
{    
}

MessageRequestState::~MessageRequestState()
{
}

int MessageRequestState::addRequest (RequestInfo &reqInfo,
                                     NOMADSUtil::UInt32RangeDLList *pMsgSeqIDs,
                                     MessageReassembler *pMsgReassembler)
{
    if (pMsgSeqIDs == NULL) {
        return -1;
    }

    uint32 ui32BeginEl, ui32EndEl;
    int rc = pMsgSeqIDs->getFirst (ui32BeginEl, ui32EndEl, RESET_GET);
    while (rc == 0) {
        addRequestInternal (reqInfo, ui32BeginEl, ui32EndEl, pMsgReassembler);
        rc = pMsgSeqIDs->getNext (ui32BeginEl, ui32EndEl);
    }

    return 0;
}

int MessageRequestState::addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                                     MessageReassembler *pMsgReassembler)
{
    int rc = addRequestInternal (reqInfo, ui32MsgSeqId, ui32MsgSeqId, pMsgReassembler);
    return rc;
}

int MessageRequestState::addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqIdStart,
                                     uint32 ui32MsgSeqIdEnd, MessageReassembler *pMsgReassembler)
{
    int rc = addRequestInternal (reqInfo, ui32MsgSeqIdStart, ui32MsgSeqIdEnd,
                                 pMsgReassembler);
    return rc;
}

int MessageRequestState::addRequestInternal (RequestInfo &reqInfo, uint32 ui32MsgSeqIdStart,
                                             uint32 ui32MsgSeqIdEnd, MessageReassembler *pMsgReassembler)
{
    if (pMsgReassembler == NULL) {
        return -1;
    }

    ByGroup *pByGroup = _states.get (reqInfo._pszGroupName);
    if (pByGroup == NULL) {
        pByGroup = new ByGroup();
        _states.put (reqInfo._pszGroupName, pByGroup);
    }

    AbstractState *pAState = pByGroup->_statesBySender.get (reqInfo._pszSenderNodeId);
    State *pState = NULL;
    if (pAState == NULL) {
        pState = new State();
        pByGroup->_statesBySender.put (reqInfo._pszSenderNodeId, pState);
    }
    else {
        assert (pAState->getType() == MSG);
        pState = (State *) pAState;
    }

    for (uint32 ui32MsgSeqId = ui32MsgSeqIdStart; ui32MsgSeqId <= ui32MsgSeqIdEnd; ui32MsgSeqId++) {
        if (!pMsgReassembler->containsMessage (reqInfo._pszGroupName, reqInfo._pszSenderNodeId, ui32MsgSeqId)) {
            pState->_msgSeqIDs.addTSN (ui32MsgSeqId);
            RequestDetails *pOldDetails = pState->_expTimes.get (ui32MsgSeqId);
            if (pOldDetails == NULL) {
                RequestDetails *pDetails = new RequestDetails (reqInfo._pszQueryId, reqInfo._ui16ClientId, reqInfo._ui64ExpirationTime);
                if (pDetails != NULL) {
                    pState->_expTimes.put (ui32MsgSeqId, pDetails);
                }
            }
            else {
                pOldDetails->addDetails (reqInfo._pszQueryId, reqInfo._ui16ClientId, reqInfo._ui64ExpirationTime);
            }

            checkAndLogMsg ("MessageRequestState::addRequestInternal", Logger::L_LowDetailDebug,
                            "range [%u, %u] for subscription %s:%s pushed into the queue (queryId <%s>).\n",
                            ui32MsgSeqIdStart, ui32MsgSeqIdEnd, reqInfo._pszGroupName, reqInfo._pszSenderNodeId,
                            reqInfo._pszQueryId == NULL ? "" : reqInfo._pszQueryId);
        }
    }
    
    return 0;
}

RequestDetails * MessageRequestState::messageArrived (MessageInfo *pMI)
{
    if (pMI == NULL) {
        return NULL;
    }
    if (!pMI->isCompleteMessage()) {
        return NULL;
    }
    ByGroup *pByGroup = _states.get (pMI->getGroupName());
    if (pByGroup == NULL) {
        return NULL;
    }

    AbstractState *pAState = pByGroup->_statesBySender.get (pMI->getPublisherNodeId());
    if (pAState == NULL) {
        return NULL;
    }
    if (pAState->getType() != MSG) {
        return NULL;
    }
    State *pState = (State *) pAState;
    pState->_msgSeqIDs.removeTSN (pMI->getMsgSeqId());
    RequestDetails *pDetails = pState->_expTimes.remove (pMI->getMsgSeqId());
    if (pDetails != NULL && !pDetails->isReplyToSearch()) {
        // No need to return the details
        delete pDetails;
        pDetails = NULL;
    }

    checkAndLogMsg ("MessageRequestState::messageArrived", Logger::L_LowDetailDebug,
                    "message %s removed from the queue\n", pMI->getMsgId());

    return pDetails;
}

void MessageRequestState::getMissingMessageRequests (MessageRequestScheduler *pReqScheduler,
                                                      MessageReassembler *pMessageReassembler)
{
    for (StringHashtable<ByGroup>::Iterator iGroup = _states.getAllElements(); !iGroup.end(); iGroup.nextElement()) {
        ByGroup *pBG = iGroup.getValue();
        const char *pszGroupName = iGroup.getKey();
        for (StringHashtable<AbstractState>::Iterator iSender = pBG->_statesBySender.getAllElements(); !iSender.end(); iSender.nextElement()) {
            if (iSender.getValue()->getType() == MSG) {
                State *pState = (State *) iSender.getValue();
                const char *pszSenderNodeId = iSender.getKey();
                pState->fillUpMissingMessages (pReqScheduler, pMessageReassembler, pszGroupName,
                                               pszSenderNodeId);
            }
        }
    }
}

bool MessageRequestState::wasRequested (const char *pszGroupName, const char *pszPublisherNodeI,
                                        uint32 ui32MsgSeqId)
{
    if (pszGroupName == NULL || pszPublisherNodeI == NULL) {
        return false;
    }
 
    ByGroup *pByGroup = _states.get (pszGroupName);
    if (pByGroup == NULL) {
        return false;
    }

    AbstractState *pAState = pByGroup->_statesBySender.get (pszPublisherNodeI);
    if (pAState == NULL) {
        return false;
    }
    assert (pAState->getType() == MSG);
    State *pState = (State *) pAState;

    return pState->_msgSeqIDs.hasTSN (ui32MsgSeqId);
}

//------------------------------------------------------------------------------
// ChunkRequestsState
//------------------------------------------------------------------------------

ChunkRequestsState::ChunkRequestsState()
{
}

ChunkRequestsState::~ChunkRequestsState()
{
}

int ChunkRequestsState::addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                                    UInt8RangeDLList *pChunkIDs,
                                    MessageReassembler *pMsgReassembler)
{
    if (pChunkIDs == NULL) {
        return -1;
    }

    uint8 ui8BeginEl, uiEndEl;
    int rc = pChunkIDs->getFirst (ui8BeginEl, uiEndEl, RESET_GET);
    while (rc == 0) {
        addRequestInternal (reqInfo, ui32MsgSeqId, ui8BeginEl, uiEndEl, pMsgReassembler);

        rc = pChunkIDs->getNext (ui8BeginEl, uiEndEl);
    }

    return 0;
}

int ChunkRequestsState::addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                                    uint8 ui8ChunkId, MessageReassembler *pMsgReassembler)
{
    int rc = addRequestInternal (reqInfo, ui32MsgSeqId, ui8ChunkId, ui8ChunkId,
                                 pMsgReassembler);
    return rc;
}

int ChunkRequestsState::addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId, uint8 ui8ChunkIdStart,
                                    uint8 ui8ChunkIdEnd, MessageReassembler *pMsgReassembler)
{
    int rc = addRequestInternal (reqInfo, ui32MsgSeqId, ui8ChunkIdStart,
                                 ui8ChunkIdEnd, pMsgReassembler);
    return rc;
}

RequestDetails * ChunkRequestsState::messageArrived (ChunkMsgInfo *pCMI)
{
    if (pCMI == NULL) {
        return NULL;
    }
    if (!pCMI->isCompleteMessage()) {
        return NULL;
    }
    ByGroup *pByGroup = _states.get (pCMI->getGroupName());
    if (pByGroup == NULL) {
        return NULL;
    }

    AbstractState *pAState = pByGroup->_statesBySender.get (pCMI->getPublisherNodeId());
    if (pAState == NULL) {
        return NULL;
    }
    assert (pAState->getType() == CHUNKED_MSG);
    ChunkedMessageState *pState = (ChunkedMessageState *) pAState;
    ChunkState *pCState = pState->_msgSeqIDs.get (pCMI->getMsgSeqId());
    if (pCState == NULL) {
        return NULL;
    }

    pCState->_chunkIDs.removeTSN (pCMI->getChunkId());
    RequestDetails *pDetails = pCState->_expTimes.remove (pCMI->getChunkId());
    if (pDetails != NULL && !pDetails->isReplyToSearch()) {
        delete pDetails;
        pDetails = NULL;
    }

    checkAndLogMsg ("ChunkRequestsState::messageArrived", Logger::L_LowDetailDebug,
                    "message %s removed from the queue\n", pCMI->getMsgId());

    return pDetails;
}

int ChunkRequestsState::addRequestInternal (RequestInfo &reqInfo, uint32 ui32MsgSeqId, uint8 ui8ChunkIdStart,
                                            uint8 ui8ChunkIdEnd, MessageReassembler *pMsgReassembler)
{
    if (pMsgReassembler == NULL) {
        return -1;
    }

    ByGroup *pByGroup = _states.get (reqInfo._pszGroupName);
    if (pByGroup == NULL) {
        pByGroup = new ByGroup();
        _states.put (reqInfo._pszGroupName, pByGroup);
    }

    ChunkedMessageState *pCMState = NULL;
    AbstractState *pState = pByGroup->_statesBySender.get (reqInfo._pszSenderNodeId);
    if (pState == NULL) {
        pCMState = new ChunkedMessageState();
        pByGroup->_statesBySender.put (reqInfo._pszSenderNodeId, pCMState);
    }
    else {
        assert (pState->getType() == CHUNKED_MSG);
        pCMState = (ChunkedMessageState *) pState;
    }

    ChunkState *pCState = pCMState->_msgSeqIDs.get (ui32MsgSeqId);
    if (pCState == NULL) {
        pCState = new ChunkState();
        pCMState->_msgSeqIDs.put (ui32MsgSeqId, pCState);
    }

    for (uint8 ui8ChunkId = ui8ChunkIdStart; ui8ChunkId <= ui8ChunkIdEnd; ui8ChunkId++) {
        if (!pMsgReassembler->containsMessage (reqInfo._pszGroupName, reqInfo._pszSenderNodeId,
                                               ui32MsgSeqId, ui8ChunkId)) {
            pCState->_chunkIDs.addTSN (ui8ChunkId);
            RequestDetails *pOldDetails = pCState->_expTimes.get (ui8ChunkId);
            if (pOldDetails == NULL) {
                RequestDetails *pDetails = new RequestDetails (reqInfo._pszQueryId, reqInfo._ui16ClientId, reqInfo._ui64ExpirationTime);
                if (pDetails != NULL) {
                    pCState->_expTimes.put (ui8ChunkId, pDetails);
                }
            }
            else {
                pOldDetails->addDetails (reqInfo._pszQueryId, reqInfo._ui16ClientId, reqInfo._ui64ExpirationTime);
            }
 
            checkAndLogMsg ("ChunkRequestsState::addRequestInternal", Logger::L_LowDetailDebug,
                            "range [%u, %u] for subscription %s:%s:%u pushed into the queue (queryId <%s>).\n",
                            ui8ChunkIdStart, ui8ChunkIdEnd, reqInfo._pszGroupName, reqInfo._pszSenderNodeId,
                            ui32MsgSeqId, reqInfo._pszQueryId == NULL ? "" : reqInfo._pszQueryId);
        }
    }

    return 0;
}

void ChunkRequestsState::getMissingMessageRequests (MessageRequestScheduler *pReqScheduler,
                                                    MessageReassembler *pMessageReassembler)
{
   ByGroup *pBG;
    const char *pszGroupName;
    for (StringHashtable<ByGroup>::Iterator iGroup = _states.getAllElements(); !iGroup.end(); iGroup.nextElement()) {
        pBG = iGroup.getValue();
        pszGroupName = iGroup.getKey();
        for (StringHashtable<AbstractState>::Iterator iSender = pBG->_statesBySender.getAllElements(); !iSender.end(); iSender.nextElement()) {
            AbstractState *pState = iSender.getValue();
            if (pState->getType() != CHUNKED_MSG) {
                continue;
            }
            const char *pszSenderNodeId = iSender.getKey();
            ChunkedMessageState *pCMState = (ChunkedMessageState *) pState;
            for (UInt32Hashtable<ChunkState>::Iterator iChState = pCMState->_msgSeqIDs.getAllElements(); !iChState.end(); iChState.nextElement()) {
                ChunkState *pState = iChState.getValue();
                pState->fillUpMissingMessages (pReqScheduler, pMessageReassembler, pszGroupName, pszSenderNodeId, iChState.getKey());
            }
        }
    }
}

bool ChunkRequestsState::wasRequested (const char *pszGroupName, const char *pszPublisherNodeId,
                                       uint32 ui32MsgSeqId, uint8 ui8ChunkId)
{
    if (pszGroupName == NULL || pszPublisherNodeId == NULL) {
        return false;
    }
    ByGroup *pByGroup = _states.get (pszGroupName);
    if (pByGroup == NULL) {
        return false;
    }

    AbstractState *pAState = pByGroup->_statesBySender.get (pszPublisherNodeId);
    if (pAState == NULL) {
        return false;
    }
    assert (pAState->getType() == CHUNKED_MSG);
    ChunkedMessageState *pState = (ChunkedMessageState *) pAState;
    ChunkState *pCState = pState->_msgSeqIDs.get (ui32MsgSeqId);
    if (pCState == NULL) {
        return false;
    }

    return pCState->_chunkIDs.hasTSN (ui8ChunkId);
}

//------------------------------------------------------------------------------
// ByGroup
//------------------------------------------------------------------------------

AbstractRequestsState::ByGroup::ByGroup()
    : _statesBySender (true,  // bCaseSensitiveKeys
                       true,  // bCloneKeys
                       true,  // bDeleteKeys
                       true)  // bDeleteValues
{   
}

AbstractRequestsState::ByGroup::~ByGroup()
{
}

//------------------------------------------------------------------------------
// AbstractState
//-----------------------------------------------------------------------------

AbstractRequestsState::AbstractState::AbstractState (StateType type)
{
    _type = type;
}

AbstractRequestsState::AbstractState::~AbstractState()
{
}

AbstractRequestsState::StateType AbstractRequestsState::AbstractState::getType()
{
    return _type;
}

//------------------------------------------------------------------------------
// State
//------------------------------------------------------------------------------

MessageRequestState::State::State()
    : AbstractState (MSG), _expTimes (true),  // bDelValues
      _msgSeqIDs (true)  // bUseSequentialArithmetic
{
}

MessageRequestState::State::~State()
{
}

void MessageRequestState::State::fillUpMissingMessages (MessageRequestScheduler *pReqScheduler,
                                                        MessageReassembler *pMessageReassembler,
                                                        const char *pszGroupName, const char *pszSenderNodeId)
{
    const char *pszMethodName = "MessageRequestState::State::fillUpMissingMessages";
    uint32 ui32BeginEl, ui32EndEl;
    int rc = _msgSeqIDs.getFirst (ui32BeginEl, ui32EndEl, RESET_GET);
    while (rc == 0) {
        for (uint32 ui32MsgSeqId = ui32BeginEl; ui32MsgSeqId <= ui32EndEl; ui32MsgSeqId++) {
            RequestDetails *pDetails = _expTimes.get (ui32MsgSeqId);
            if (pDetails != NULL && pDetails->_i64ExpirationTime > 0 && pDetails->_i64ExpirationTime < getTimeInMilliseconds()) {
                checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                                "NOT requesting %s:%s:%u because the request expired\n",
                                pszGroupName, pszSenderNodeId, ui32MsgSeqId);
                continue;
            }
            if (!pMessageReassembler->containsMessage (pszGroupName, pszSenderNodeId, ui32MsgSeqId)) {
                // create and add the request
                checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "Requesting %s:%s:%u\n",
                                pszGroupName, pszSenderNodeId, ui32MsgSeqId);
                pReqScheduler->addRequest (new DisServiceDataReqMsg::MessageRequest (pszGroupName, pszSenderNodeId, ui32MsgSeqId), false, true);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                                "NOT requesting %s:%s:%u because already in the MessageReassembler\n",
                                pszGroupName, pszSenderNodeId, ui32MsgSeqId);
            }
        }
        rc = _msgSeqIDs.getNext (ui32BeginEl, ui32EndEl);
    }
}

//------------------------------------------------------------------------------
// ChunkedMessageState
//------------------------------------------------------------------------------

ChunkRequestsState::ChunkedMessageState::ChunkedMessageState()
    : AbstractState (CHUNKED_MSG)
{
}

ChunkRequestsState::ChunkedMessageState::~ChunkedMessageState()
{
}

//------------------------------------------------------------------------------
// ChunkRequestsState
//------------------------------------------------------------------------------

ChunkRequestsState::ChunkState::ChunkState()
    : _expTimes (true),  // bDelValues
      _chunkIDs (false)  // bUseSequentialArithmetic
{
}

ChunkRequestsState::ChunkState::~ChunkState()
{
}

void ChunkRequestsState::ChunkState::fillUpMissingMessages (MessageRequestScheduler *pReqScheduler,
                                                            MessageReassembler *pMessageReassembler,
                                                            const char *pszGroupName, const char *pszSenderNodeId,
                                                            uint32 ui32SeqId)
{
    const char *pszMethodName = "ChunkRequestsState::ChunkState::fillUpMissingMessages";
    uint8 ui8BeginEl, ui8EndEl;
    int rc = _chunkIDs.getFirst (ui8BeginEl, ui8EndEl, RESET_GET);
    while (rc == 0) {
        for (uint8 ui8ChunkId = ui8BeginEl; ui8ChunkId <= ui8EndEl; ui8ChunkId++) {
            RequestDetails *pDetails = _expTimes.get (ui8ChunkId);
            if (pDetails != NULL && (pDetails->_i64ExpirationTime) > 0 && (pDetails->_i64ExpirationTime) < getTimeInMilliseconds()) {
                checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                                "NOT requesting %s:%s:%u:%u because the request expired\n",
                                pszGroupName, pszSenderNodeId, ui32SeqId, ui8ChunkId);
                continue;
            }
            if (!pMessageReassembler->containsMessage (pszGroupName, pszSenderNodeId, ui32SeqId, ui8ChunkId)) {
                // create and add the request
                checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "Requesting %s:%s:%u:%u\n",
                                pszGroupName, pszSenderNodeId, ui32SeqId, ui8ChunkId);
                pReqScheduler->addRequest (new DisServiceDataReqMsg::ChunkMessageRequest (pszGroupName, pszSenderNodeId, ui32SeqId, ui8ChunkId), false, true);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                                "NOT requesting %s:%s:%u:%u because already in the MessageReassembler\n",
                                pszGroupName, pszSenderNodeId, ui32SeqId, ui8ChunkId);
            }
        }
        rc = _chunkIDs.getNext (ui8BeginEl, ui8EndEl);
    }
}

//--------------------------------------------------------------------------
// RequestsState
//--------------------------------------------------------------------------

RequestsState::RequestsState()
    : _m (25)
{
}

RequestsState::~RequestsState()
{
}

int RequestsState::addRequest (RequestInfo &reqInfo,
                               NOMADSUtil::UInt32RangeDLList *pMsgSeqIDs,
                               MessageReassembler *pMsgReassembler)
{
    _m.lock (190);
    int rc = _msgReqState.addRequest (reqInfo, pMsgSeqIDs, pMsgReassembler);
    _m.unlock (190);
    return rc;
}

int RequestsState::addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                               MessageReassembler *pMsgReassembler)
{
    _m.lock (191);
    int rc = _msgReqState.addRequest (reqInfo, ui32MsgSeqId, pMsgReassembler);
    _m.unlock (191);
    return rc;
}

int RequestsState::addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqIdStart,
                               uint32 ui32MsgSeqIdEnd, MessageReassembler *pMsgReassembler)
{
    _m.lock (192);
    int rc = _msgReqState.addRequest (reqInfo, ui32MsgSeqIdStart, ui32MsgSeqIdEnd,
                                      pMsgReassembler);
    _m.unlock (192);
    return rc;
}

int RequestsState::addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                               NOMADSUtil::UInt8RangeDLList *pChunkIDs,
                               MessageReassembler *pMsgReassembler)
{
    _m.lock (193);
    int rc = _chunkReqState.addRequest (reqInfo, ui32MsgSeqId, pChunkIDs, pMsgReassembler);
    _m.unlock (193);
    return rc;
}

int RequestsState::addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                               uint8 ui8ChunkId, MessageReassembler *pMsgReassembler)
{
    _m.lock (194);
    int rc = _chunkReqState.addRequest (reqInfo, ui32MsgSeqId, ui8ChunkId, pMsgReassembler);
    _m.unlock (194);
    return rc;
}

int RequestsState::addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                               uint8 ui8ChunkIdStart, uint8 ui8ChunkIdEnd,
                               MessageReassembler *pMsgReassembler)
{
    _m.lock (195);
    int rc = _chunkReqState.addRequest (reqInfo, ui32MsgSeqId, ui8ChunkIdStart,
                                        ui8ChunkIdEnd, pMsgReassembler);
    _m.unlock (195);
    return rc;
}

RequestDetails * RequestsState::messageArrived (Message *pMsg)
{
    if (pMsg == NULL) {
        return NULL;
    }
    MessageHeader *pMH = pMsg->getMessageHeader();
    if (pMH == NULL) {;
        return NULL;
    }
    if (pMH->isChunk()) {
        _m.lock (196);
        RequestDetails *pDetails = _chunkReqState.messageArrived ((ChunkMsgInfo*) pMH);
        _m.unlock (196);
        return pDetails;
    }

    _m.lock (197);
    RequestDetails *pDetails = _msgReqState.messageArrived ((MessageInfo*) pMH);
    _m.unlock (197);
    return pDetails;
}

RequestDetails * RequestsState::messageArrived (Message *pMsg, bool)
{
    return messageArrived (pMsg);
}

void RequestsState::getMissingMessageRequests (MessageRequestScheduler *pReqScheduler,
                                               MessageReassembler *pMessageReassembler)
{
    _m.lock (198);
    _msgReqState.getMissingMessageRequests (pReqScheduler, pMessageReassembler);
    _chunkReqState.getMissingMessageRequests (pReqScheduler, pMessageReassembler);
    _m.unlock (198);
}

bool RequestsState::wasRequested (Message *pMsg)
{
    if (pMsg == NULL) {
        return false;
    }
    return wasRequested (pMsg->getMessageHeader());
}

bool RequestsState::wasRequested (MessageHeader *pMH)
{
    if (pMH == NULL) {
        return false;
    }
    _m.lock (197);
    bool bWasRequested = _chunkReqState.wasRequested (pMH->getGroupName(), pMH->getPublisherNodeId(),
                                                      pMH->getMsgSeqId(), pMH->getChunkId());
    _m.unlock (197);
    return bWasRequested;
}

bool RequestsState::wasRequested (const char *pszGroupName, const char *pszPublisherNodeId,
                                  uint32 ui32MsgSeqId, uint8 ui8ChunkId)
{
    if (pszGroupName == NULL || pszPublisherNodeId == NULL) {
        return false;
    }

    if (ui8ChunkId == MessageHeader::UNDEFINED_CHUNK_ID) {
        return _msgReqState.wasRequested (pszGroupName, pszPublisherNodeId, ui32MsgSeqId);
        
    }
    return _chunkReqState.wasRequested (pszGroupName, pszPublisherNodeId,
                                        ui32MsgSeqId, ui8ChunkId);
}

RequestInfo::RequestInfo (uint16 ui16ClientId)
    : _ui16ClientId (ui16ClientId)
{
    _pszGroupName = 0;
    _pszSenderNodeId = NULL;
    _ui64ExpirationTime = 0;
    _pszQueryId = NULL;
}

RequestInfo::~RequestInfo (void)
{
}

RequestDetails::RequestDetails (const char *pszQueryId, uint16 ui16ClientId, int64 i64ExpirationTime)
    : _i64ExpirationTime (i64ExpirationTime)
{
    QueryDetails *pDetails = new QueryDetails (pszQueryId, ui16ClientId, i64ExpirationTime);
    if (pDetails != NULL) {
        _details.append (pDetails);
    }
}

RequestDetails::~RequestDetails (void)
{
    QueryDetails *pCurr, *pNext;
    pNext = _details.getFirst();
    while ((pCurr = pNext) != NULL) {
        pNext = _details.getNext();
        delete _details.remove (pCurr);
    }
}

void RequestDetails::addDetails (const char *pszQueryId, uint16 ui16ClientId, int64 i64ExpirationTime)
{
    if (i64ExpirationTime > _i64ExpirationTime) {
        _i64ExpirationTime = i64ExpirationTime;
    }

    QueryDetails *pNewDetails = new QueryDetails (pszQueryId, ui16ClientId, i64ExpirationTime);
    if (pNewDetails == NULL) {
        return;
    }
    QueryDetails *pOldDetails = _details.search (pNewDetails);
    if (pOldDetails == NULL) {
        _details.prepend (pNewDetails);
    }
    else if (pOldDetails->_i64ExpirationTime < pNewDetails->_i64ExpirationTime) {
        pOldDetails->_i64ExpirationTime = pNewDetails->_i64ExpirationTime;
    }
}

bool RequestDetails::isReplyToSearch (void)
{
    return !_details.isEmpty();
}

RequestDetails::QueryDetails::QueryDetails (const char *pszQueryId, uint16 ui16ClientId, int64 i64ExpirationTime)
    : _ui16ClientId (ui16ClientId),
      _i64ExpirationTime (i64ExpirationTime),
      _queryId (pszQueryId)
{
}

RequestDetails::QueryDetails::~QueryDetails (void)
{
}

int RequestDetails::QueryDetails::operator == (const QueryDetails &rhsDetails) const
{
    return ((_queryId == rhsDetails._queryId) && (_ui16ClientId == rhsDetails._ui16ClientId));
}

