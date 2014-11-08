/*
 * MessageReassembler.cpp
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

#include "MessageReassembler.h"

#include "DataCacheInterface.h"
#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "DisServiceStats.h"
#include "Message.h"
#include "MessageId.h"
#include "NodeInfo.h"
#include "PeerState.h"
#include "SubscriptionState.h"
#include "TransmissionService.h"

#include "Logger.h"
#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const bool MessageReassembler::DEFAULT_ENABLE_EXPONENTIAL_BACKOFF = false;

const int MessageReassembler::UNLIMITED_MAX_NUMBER_OF_REQUESTS = -1;
const int MessageReassembler::DEFAULT_MAX_NUMBER_OF_REQUESTS = UNLIMITED_MAX_NUMBER_OF_REQUESTS;
const float MessageReassembler::DEFAULT_RANGE_REQUEST_PROB = 100.0f;
const float MessageReassembler::DEFAULT_RECEIVE_RATE_THRESHOLD = 0.80f;
const uint8 MessageReassembler::ALWAYS_REQUEST_RANGES_THREASHOLD = 10;
const uint32 MessageReassembler::DEFAULT_INCOMING_QUEUE_SIZE_REQUEST_THRESHOLD = 15;
const bool MessageReassembler::DEFAULT_REQUEST_FRAGMENTS_FOR_OPPORTUNISTICALLY_ACQUIRED_MESSAGES = false;

bool checkCompleteMessageLength (MessageHeader *pMI, uint32 ui32DataLength, bool bIsChunk, bool bIsMetaDataPart);

MessageReassembler::MessageReassembler (DisseminationService *pDisService, PeerState *pPeerState,
                                        SubscriptionState *pSubState, LocalNodeInfo *pLocalNodeInfo,
                                        float fDefaultReqProb, float fReceiveRateThreshold,
                                        bool bExpBackoff, bool bReqFragmentsForOpporAcquiredMsgs)
    : _receivedMessages (true,  // bCaseSensitiveKeys
                         true,  // bCloneKeys
                         true,  // bDeleteKeys
                         true), // bDeleteValues
      _m (20),
      _newPeerMutex (21),
      _requestSched (fDefaultReqProb)
{
    if (pDisService == NULL || pPeerState == NULL ||
        pSubState == NULL || pLocalNodeInfo == NULL) {
        checkAndLogMsg ("MessageReassembler::MessageReassembler", coreComponentInitFailed);
        exit (1);
    }

    _pDisService = pDisService;
    _pPeerState = pPeerState;
    _pLocalNodeInfo = pLocalNodeInfo;
    _pSubState = pSubState;
    _pTrSvc = NULL;
    _bNewPeer = false;
    _bExpBackoff = bExpBackoff;
    _iMaxNumberOfReqs = DEFAULT_MAX_NUMBER_OF_REQUESTS;
    _fRangeRequestProb = fDefaultReqProb;
    _ui32IncomingQueueSizeRequestThreshold = DEFAULT_INCOMING_QUEUE_SIZE_REQUEST_THRESHOLD;
    _fReceiveRateThreshold = fReceiveRateThreshold;
    _bReqFragmentsForOpporAcquiredMsgs = bReqFragmentsForOpporAcquiredMsgs;

    // initialize random seed:
    srand ((unsigned int)getTimeInMilliseconds());
}

MessageReassembler::~MessageReassembler (void)
{
    if (isRunning()) {
        requestTerminationAndWait();
    }

    _pDisService = NULL;
    _pLocalNodeInfo = NULL;
    _pSubState = NULL;
}

bool MessageReassembler::usingExponentialBackOff()
{
    return _bExpBackoff;
}

int MessageReassembler::setRequestLimit (int iLimit)
{
    if (iLimit < UNLIMITED_MAX_NUMBER_OF_REQUESTS) {
        return -1;
    }
    _iMaxNumberOfReqs = iLimit;
    return 0;
}

int MessageReassembler::getRequestLimit()
{
    return _iMaxNumberOfReqs;
}

void MessageReassembler::setRequestProbability (uint8 ui8Priority, float lReqProb)
{
    _requestSched.setRequestProbability (ui8Priority, lReqProb);
}

float MessageReassembler::getRequestProbability (uint8 ui8Priority) const
{
    return _requestSched.getRequestProbability (ui8Priority);
}

int MessageReassembler::addRequest (RequestInfo &reqInfo, UInt32RangeDLList *pMsgSeqIDs)
{
    _m.lock (175);
    int rc = _requestState.addRequest (reqInfo, pMsgSeqIDs, this);

    MessagesByGroup *pMG = _receivedMessages.get (reqInfo._pszGroupName);
    if (pMG == NULL) {
        _m.unlock (175);
        return false;
    }

    MessagesByPublisher *pMS = pMG->messagesInGroup.get (reqInfo._pszSenderNodeId);
    if (pMS == NULL) {
         _m.unlock (175);
        return false;
    }

    static const bool RESET_GET = true;
    uint32 ui32BeginEl, ui32EndEl;
    rc = pMsgSeqIDs->getFirst (ui32BeginEl, ui32EndEl, RESET_GET);
    while (rc == 0) {
        for (uint32 ui32MsgSeqId = ui32BeginEl; ui32MsgSeqId <= ui32EndEl; ui32MsgSeqId++) {
            FragmentedMessage searchTemplate (ui32MsgSeqId, MessageInfo::UNDEFINED_CHUNK_ID);
            FragmentedMessage *pFragMsg = pMS->messages.search (&searchTemplate);
            if (pFragMsg != NULL) {
                if (pFragMsg->pRequestDetails == NULL) {
                    pFragMsg->pRequestDetails = new RequestDetails (reqInfo._pszQueryId, reqInfo._ui16ClientId, reqInfo._ui64ExpirationTime);
                }
            }
        }
        rc = pMsgSeqIDs->getNext (ui32BeginEl, ui32EndEl);
    }

    _m.unlock (175);
    return rc;
}

int MessageReassembler::addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                                    NOMADSUtil::UInt8RangeDLList *pChunkIDs)
{
    _m.lock (176);
    int rc = _requestState.addRequest (reqInfo, ui32MsgSeqId, pChunkIDs, this);

    MessagesByGroup *pMG = _receivedMessages.get (reqInfo._pszGroupName);
    if (pMG == NULL) {
        _m.unlock (176);
        return false;
    }

    MessagesByPublisher *pMS = pMG->messagesInGroup.get (reqInfo._pszSenderNodeId);
    if (pMS == NULL) {
         _m.unlock (176);
        return false;
    }

    static const bool RESET_GET = true;
    uint8 ui8BeginEl, ui8EndEl;
    rc = pChunkIDs->getFirst (ui8BeginEl, ui8EndEl, RESET_GET);
    while (rc == 0) {
        for (uint8 ui8ChunkId = ui8BeginEl; ui8ChunkId <= ui8EndEl; ui8ChunkId++) {
            FragmentedMessage searchTemplate (ui8ChunkId, MessageInfo::UNDEFINED_CHUNK_ID);
            FragmentedMessage *pFragMsg = pMS->messages.search (&searchTemplate);
            if (pFragMsg != NULL) {
                if (pFragMsg->pRequestDetails == NULL) {
                    pFragMsg->pRequestDetails = new RequestDetails (reqInfo._pszQueryId, reqInfo._ui16ClientId, reqInfo._ui64ExpirationTime);
                }
            }
        }
        rc = pChunkIDs->getNext (ui8BeginEl, ui8EndEl);
    }

    _m.unlock (176);
    return rc;
}

bool MessageReassembler::containsMessage (const char *pszGroupName, const char *pszSenderNodeId,
                                          uint32 ui32MsgSeqId)
{
    return containsMessage (pszGroupName, pszSenderNodeId, ui32MsgSeqId,
                            MessageHeader::UNDEFINED_CHUNK_ID);
}

bool MessageReassembler::containsMessage (const char *pszGroupName, const char *pszSenderNodeId,
                                          uint32 ui32MsgSeqId, uint8 ui8ChunkId)
{
    _m.lock (177);
    MessagesByGroup *pMG = _receivedMessages.get (pszGroupName);
    if (pMG == NULL) {
        _m.unlock (177);
        return false;
    }

    MessagesByPublisher *pMS = pMG->messagesInGroup.get (pszSenderNodeId);
    if (pMS == NULL) {
         _m.unlock (177);
        return false;
    }

    FragmentedMessage searchTemplate (ui32MsgSeqId, ui8ChunkId);
    FragmentedMessage *pFragMsg = pMS->messages.search (&searchTemplate);
    if (pFragMsg == NULL) {
        _m.unlock (177);
        return false;
    }

    _m.unlock (177);
    return true;
}

bool MessageReassembler::containsMessage (MessageId *pMsgId)
{
    if (pMsgId == NULL) {
        return false;
    }
    return containsMessage (pMsgId->getGroupName(), pMsgId->getOriginatorNodeId(),
                            pMsgId->getSeqId(), pMsgId->getChunkId());
}

bool MessageReassembler::containsMessage (MessageHeader *pMI, bool bSpecificFragment)
{
    _m.lock (178);
    MessagesByGroup *pMG = _receivedMessages.get (pMI->getGroupName());
    if (pMG == NULL) {
        _m.unlock (178);
        return false;
    }

    MessagesByPublisher *pMS = pMG->messagesInGroup.get (pMI->getPublisherNodeId());
    if (pMS == NULL) {
         _m.unlock (178);
        return false;
    }

    FragmentedMessage searchTemplate (pMI->getMsgSeqId(), pMI->getChunkId());
    FragmentedMessage *pFragMsg = pMS->messages.search (&searchTemplate);
    if (pFragMsg == NULL) {
         _m.unlock (178);
        return false;
    }

    if (!bSpecificFragment) {
         _m.unlock (178);
        return true;
    }

    FragmentWrapper *pFragWrapper = new FragmentWrapper (pMI->getFragmentOffset(), pMI->getFragmentLength(), pMI->isChunk());
    if ((pFragMsg == NULL) || (pFragMsg->searchFragment (*pFragWrapper) == NULL)) {
        delete pFragWrapper;
        pFragWrapper = NULL;
         _m.unlock (178);
        return false;
    }

    delete pFragWrapper;
    pFragWrapper = NULL;
    _m.unlock (178);
    return true;
}

bool MessageReassembler::hasFragment (const char *pszGroupName, const char *pszSenderNodeId,
                                      uint32 ui32MsgSeqId, uint8 ui8ChunkId, uint32 ui32FragOffset, uint16 ui16FragLength)
{
    if ((pszGroupName == NULL) || (pszSenderNodeId == NULL)) {
        checkAndLogMsg ("MessageReassembler::hasFragment", Logger::L_MildError,
                        "pszGroupName or pszSenderNodeId is NULL\n");
        return false;
    }

    _m.lock (0);
    MessagesByGroup *pMG = _receivedMessages.get (pszGroupName);
    if (pMG == NULL) {
        _m.unlock (0);
        return false;
    }

    MessagesByPublisher *pMS = pMG->messagesInGroup.get (pszSenderNodeId);
    if (pMS == NULL) {
         _m.unlock (0);
        return false;
    }

    FragmentedMessage searchTemplate (ui32MsgSeqId, ui8ChunkId);
    FragmentedMessage *pFragMsg = pMS->messages.search (&searchTemplate);
    if (pFragMsg == NULL) {
         _m.unlock (0);
        return false;
    }

    // If we have got to this point, now we need to search within the fragments to make sure that the entire range is contained
    FragmentWrapper *pFragWrapper = pFragMsg->getFirstFragment();
    while (pFragWrapper != NULL) {
        if (pFragWrapper->ui32FragmentOffset <= ui32FragOffset) {
            if (pFragWrapper->ui16FragmentLength >= ui16FragLength) {
                _m.unlock (0);
                return true;
            }
            else {
                // The requested fragment was not fully contained in the previous fragment - continue checking
                uint32 ui32NewFragOffset = pFragWrapper->ui32FragmentOffset + pFragWrapper->ui16FragmentLength;
                ui16FragLength = ui16FragLength - (ui32NewFragOffset - ui32FragOffset);
                ui32FragOffset = ui32NewFragOffset;
            }
        }
        pFragWrapper = pFragMsg->getNextFragment();
    }
    _m.unlock (0);
    return false;
}

bool MessageReassembler::isBeingReassembled (const char *pszGroupName, const char *pszSenderNodeId,
                                             uint32 ui32MsgSeqId, uint8 ui8ChunkId)
{
    if (pszGroupName == NULL || pszSenderNodeId == NULL) {
        return false;
    }
    _m.lock (334);
    bool bIsBeingReassembled = isBeingReassembledInternal (pszGroupName, pszSenderNodeId,
                                                           ui32MsgSeqId, ui8ChunkId, true);
    _m.unlock (334);
    return bIsBeingReassembled;
}

bool MessageReassembler::isBeingReassembledInternal (const char *pszGroupName, const char *pszSenderNodeId,
                                                     uint32 ui32MsgSeqId, uint8 ui8ChunkId, bool bCheckRequestState)
{
    if (pszGroupName == NULL || pszSenderNodeId == NULL) {
        return false;
    }

    // Check the message reassembler first
    MessagesByGroup *pMG = _receivedMessages.get (pszGroupName);
    if (pMG == NULL) {
        return false;
    }

    MessagesByPublisher *pMS = pMG->messagesInGroup.get (pszSenderNodeId);
    if (pMS == NULL) {
        return false;
    }

    FragmentedMessage searchTemplate (ui32MsgSeqId, ui8ChunkId);
    FragmentedMessage *pFragMsg = pMS->messages.search (&searchTemplate);
    if (pFragMsg == NULL) {
        if (bCheckRequestState) {
            return _requestState.wasRequested (pszGroupName, pszSenderNodeId,
                                               ui32MsgSeqId, ui8ChunkId);
        }
        else {
            return false;
        }
    }

    return true;
}

int MessageReassembler::getSubscriptionParameters (Message *pMessage, bool bIsNotTarget, bool &isSequenced, bool &isReliable)
{
    if (!_pLocalNodeInfo->hasSubscription (pMessage)) {
        if (!_bReqFragmentsForOpporAcquiredMsgs) {
            // Not interested in this group
            return 1;
        }
        else {
            isSequenced = false;
            isReliable = true;
        }
    }
    else if (pMessage->getMessageHeader()->isChunk()) {
        isSequenced = false;
        isReliable = true;
    }
    else {
        const char *pszGroupName = pMessage->getMessageHeader()->getGroupName();
        uint16 ui16Tag = pMessage->getMessageHeader()->getTag();
        isSequenced = _pLocalNodeInfo->requireSequentiality (pszGroupName, ui16Tag);
        if (bIsNotTarget && _bReqFragmentsForOpporAcquiredMsgs) {
            // Force the message to be reassembled, regardless of what the
            // subscription says
            isReliable = true;
        }
        else {
            isReliable = _pLocalNodeInfo->requireMessageReliability (pszGroupName, ui16Tag);
        }
    }
    return 0;
}

int MessageReassembler::fragmentArrived (Message *pMessage, bool bIsNotTarget)
{
    const char *pszMethodName = "MessageReassembler::fragmentArrived";
    if (pMessage == NULL || pMessage->getMessageHeader() == NULL) {
        return -1;
    }
    if (pMessage->getMessageHeader()->getFragmentLength() == 0) {
        return -2;
    }

    const char *pszGroupName = pMessage->getMessageHeader()->getGroupName();
    const char *pszSenderNodeId = pMessage->getMessageHeader()->getPublisherNodeId();
    uint32 ui32NewMsgSeqId = pMessage->getMessageHeader()->getMsgSeqId();
    uint8 ui8NewMsgChunkId = pMessage->getMessageHeader()->getChunkId();
    const void *pFragment = pMessage->getData();
    bool isSequenced, isReliable;
    isSequenced = isReliable = true; // these are set by getSubscriptionParameters() 
                                     // but just to be safe...
    _m.lock (179);
    int rc = getSubscriptionParameters (pMessage, bIsNotTarget, isSequenced, isReliable);
    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                        "this node is not interested in fragment <%s> because group <%s> is not subscribed\n",
                        pMessage->getMessageHeader()->getMsgId(), pszGroupName);
        _m.unlock (179);
        return 1;
    }

    // Get the subscription state for the group the incoming fragment belongs to
    uint32 ui32CurrentSubState = _pSubState->getSubscriptionState (pszGroupName, pszSenderNodeId);
    bool bIsInHistory = _pLocalNodeInfo->isInAnyHistory (pMessage, ui32CurrentSubState);

    // Verify the relevance of the incoming fragment
    if (isSequenced && (ui32CurrentSubState > ui32NewMsgSeqId) && (!bIsInHistory)) {
        // Newer message(s) have already been received
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                        "this node is not interested in fragment <%s> because it is no longer relevant and it is not part of a history request\n",
                        pMessage->getMessageHeader()->getMsgId());
        _m.unlock (179);
        return 2;
    }

    // Create the FragmentWrapper
    FragmentWrapper *pFragWrapper = new FragmentWrapper (pMessage->getMessageHeader()->getFragmentOffset(),
            pMessage->getMessageHeader()->getFragmentLength(), pMessage->getMessageHeader()->isChunk());
    pFragWrapper->pFragment = (void*) pFragment;

    // Check if this is the first message received in this group
    MessagesByGroup *pMG = _receivedMessages.get (pszGroupName);
    if (NULL == pMG) {
        pMG = new MessagesByGroup();
        if (pMG == NULL) {
            // Do not delete pFragWrapper->pFragment as it the same as pMessage->getData() - caller will free
            pFragWrapper->pFragment = NULL;
            delete pFragWrapper;
            pFragWrapper = NULL;
            checkAndLogMsg (pszMethodName, memoryExhausted);
            _m.unlock (179);
            return -3;
        }
        _receivedMessages.put (pszGroupName, pMG);
    }

    // Check if this is the first message from this sender
    MessagesByPublisher *pMS = pMG->messagesInGroup.get (pszSenderNodeId);
    if (pMS == NULL) {
        pMS = new MessagesByPublisher();
        if (pMS == NULL) {
            // Do not delete pFragWrapper->pFragment as it the same as pMessage->getData() - caller will free
            pFragWrapper->pFragment = NULL;
            delete pFragWrapper;
            pFragWrapper = NULL;
            checkAndLogMsg (pszMethodName, memoryExhausted);
            _m.unlock (179);
            return -4;
        }
        pMG->messagesInGroup.put (pszSenderNodeId, pMS);
    }

    // Check if there is already a FragmentedMessage object for
    // this message (using the sequence id and chunk id of the message)
    FragmentedMessage searchTemplate (ui32NewMsgSeqId, ui8NewMsgChunkId);
    FragmentedMessage *pFragMsg = pMS->messages.search (&searchTemplate);
    if (pFragMsg == NULL) {
        // This is the first fragment for this message.
        // Create a new instance of FragmentedMessage
        pFragMsg = new FragmentedMessage (pMessage->getMessageInfo(), isReliable, isSequenced, bIsInHistory, bIsNotTarget); // FragmentedMessage makes a copy of the MessageInfo
                                                                       // NOTE: Don't use pMI henceforth
        if (pFragMsg == NULL) {
            // Do not delete pFragWrapper->pFragment as it the same as pMessage->getData() - caller will free
            pFragWrapper->pFragment = NULL;
            delete pFragWrapper;
            pFragWrapper = NULL;
            checkAndLogMsg (pszMethodName, memoryExhausted);
            _m.unlock (179);
            return -6;
        }

        pMS->messages.insert (pFragMsg);
        pFragMsg->addFragment (pFragWrapper);
        updateMissingFragmentsList (pFragMsg);
        pFragMsg->updateNextExpectedValue();
    }
    else {
        FragmentWrapper *pFoundFragWrapper = pFragMsg->searchFragment (*pFragWrapper);
        if (NULL != pFoundFragWrapper) {
            // This fragment already exists - discard
            checkAndLogMsg ("MessageReassembler::fragmentArrived", Logger::L_Warning,
                            "trying to add duplicate fragment - offset: %lu length %lu of message %s."
                            " (Found fragment of offset %lu length %lu)\n", pFragWrapper->ui32FragmentOffset,
                            pFragWrapper->ui16FragmentLength, pFragMsg->pMH->getMsgId(),
                            pFoundFragWrapper->ui32FragmentOffset, pFoundFragWrapper->ui16FragmentLength);
            // Do not delete pFragWrapper->pFragment as it the same as pMessage->getData() - caller will free
            pFragWrapper->pFragment = NULL;
            delete pFragWrapper;
            pFragWrapper = NULL;
            _m.unlock (179);
            return -7;
        }
        // Otherwise, insert fragment
        pFragMsg->addFragment (pFragWrapper);
        uint8 ui8MsgComplete = messageComplete (pFragMsg);
        if (ui8MsgComplete == MSG_COMPLETE) {
            // Reassemble the message, deliver and cache it
            uint32 ui32BytesWritten;
            void *pReassembledMsg = reassemble (pFragMsg, false, ui32BytesWritten);
            if (pReassembledMsg != NULL) {
                // deliverCompleteMessage() takes care of deleting the
                // FragmentedMessage and its content.
                deliverCompleteMessage (pFragMsg->pMH, pReassembledMsg, ui32BytesWritten,
                                        pMS, pFragMsg, false, pFragMsg->bIsNotTarget);
            }
        }
        else {
            if ((ui8MsgComplete == METADATA_COMPLETE) && (!pFragMsg->bMetaDataDelivered)) {
                uint32 ui32BytesWritten;
                void *pReassembledMsg = reassemble (pFragMsg, true, ui32BytesWritten);
                if (pReassembledMsg != NULL) {
                    deliverCompleteMessage (pFragMsg->pMH, pReassembledMsg, ui32BytesWritten,
                                            pMS, pFragMsg, true, pFragMsg->bIsNotTarget);
                }
                else {
                    // Reassembly failed - warn
                    checkAndLogMsg ("MessageReassembler::fragmentArrived", Logger::L_Warning,
                                    "reassemble failed (second case) even though message should have been complete\n");
                }
            }
            if (pFragMsg->ui32NextExpected < (pFragWrapper->ui16FragmentLength + pFragWrapper->ui32FragmentOffset)) {
                pFragMsg->ui32NextExpected = pFragWrapper->ui16FragmentLength + pFragWrapper->ui32FragmentOffset;
            }
        }
    }
    _m.unlock (179);
    return 0;
}

RequestDetails * MessageReassembler::messageArrived (Message *pMsg)
{
    _m.lock (180);
    RequestDetails *pDetails = _requestState.messageArrived (pMsg);
    _m.unlock (180);
    return pDetails;
}

void MessageReassembler::newPeer()
{
    _newPeerMutex.lock (181);
    _bNewPeer = true;
    _newPeerMutex.unlock (181);
}

void MessageReassembler::registerTransmissionService (TransmissionService *pTrSvc)
{
    _pTrSvc = pTrSvc;
}

// --- Send  request -----------------------------------------------------------

int MessageReassembler::sendRequest (RequestInfo &reqInfo, UInt32RangeDLList *pMsgSeqIDs)
{
    _m.lock (263);
    if (reqInfo._pszGroupName == NULL || reqInfo._pszSenderNodeId == NULL || pMsgSeqIDs == NULL) {
        _m.unlock (263);
        return -1;
    }
    PtrLList<FragmentRequest> *pMessageRequests = new PtrLList<FragmentRequest>();
    if (pMessageRequests == NULL) {
        checkAndLogMsg ("MessageReassembler::sendRequest", memoryExhausted);
        _m.unlock (263);
        return -2;
    }

    static const bool RESET_GET = true;
    uint32 ui32BeginEl, ui32EndEl;
    for (int rc = pMsgSeqIDs->getFirst (ui32BeginEl, ui32EndEl, RESET_GET);
         rc == 0; rc = pMsgSeqIDs->getNext (ui32BeginEl, ui32EndEl)) {

        sendRequest (reqInfo, ui32BeginEl, ui32EndEl, pMessageRequests);
    }

    DisServiceDataReqMsg req (_pDisService->getNodeId(), NULL, // pszQueryTargetNodeId
                              _pPeerState->getNumberOfActiveNeighbors(),
                              pMessageRequests, NULL);

    int rc = _pDisService->broadcastDisServiceCntrlMsg (&req, NULL, "Sending DisServiceDataReqMsg", NULL, NULL);
    _pDisService->getStats()->missingFragmentRequestSent (req.getSize());

    // Deallocate pMessageRequests and its elements
    FragmentRequest *pReq;
    FragmentRequest *pTmp= pMessageRequests->getFirst();
    while ((pReq = pTmp) != NULL) {
        pTmp= pMessageRequests->getNext();
        delete pReq;
    }
    delete pMessageRequests;

    _m.unlock (263);
    return rc;
}

int MessageReassembler::sendRequest (RequestInfo &reqInfo, uint32 ui32BeginEl, uint32 ui32EndEl,
                                     PtrLList<FragmentRequest> *pMessageRequests)
{
    _m.lock (264);
    int rc = sendRequestInternal (reqInfo, ui32BeginEl, ui32EndEl, pMessageRequests);
    _m.unlock (264);
    return rc;
}

int MessageReassembler::sendRequestInternal (RequestInfo &reqInfo, uint32 ui32BeginEl, uint32 ui32EndEl,
                                             PtrLList<FragmentRequest> *pMessageRequests)
{
    if (reqInfo._pszGroupName == NULL || reqInfo._pszSenderNodeId == NULL || pMessageRequests == NULL) {
        return -1;
    }

    for (uint32 ui32SeqId = ui32BeginEl; ui32SeqId <= ui32EndEl; ui32SeqId++) {
        if (!containsMessage (reqInfo._pszGroupName, reqInfo._pszSenderNodeId, ui32SeqId)) {
            // create and add the request
            checkAndLogMsg ("MessageReassembler::sendRequestInternal", Logger::L_LowDetailDebug,
                            "Requesting %s:%s:%u\n", reqInfo._pszGroupName, reqInfo._pszSenderNodeId, ui32SeqId);
            FragmentRequest *pFragReq = new DisServiceDataReqMsg::MessageRequest (reqInfo._pszGroupName, reqInfo._pszSenderNodeId, ui32SeqId);
            if (pFragReq == NULL) {
                checkAndLogMsg ("MessageReassembler::sendRequestInternal", memoryExhausted);
                break;
            }
            pMessageRequests->prepend (pFragReq);
        }
    }

    return 0;
}

// --- Send chunk request ------------------------------------------------------

int MessageReassembler::sendRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId, NOMADSUtil::UInt8RangeDLList *pChunkIDs)
{
    _m.lock (265);
    if (reqInfo._pszGroupName == NULL || reqInfo._pszSenderNodeId == NULL || pChunkIDs == NULL) {
        _m.unlock (265);
        return -1;
    }
    PtrLList<FragmentRequest> *pMessageRequests = new PtrLList<FragmentRequest>();
    if (pMessageRequests == NULL) {
        checkAndLogMsg ("MessageReassembler::sendRequest", memoryExhausted);
        _m.unlock (265);
        return -2;
    }

    static bool RESET_GET = true;
    uint8 ui8BeginEl, ui8EndEl;
    for (int rc = pChunkIDs->getFirst (ui8BeginEl, ui8EndEl, RESET_GET);
         rc == 0; rc = pChunkIDs->getNext (ui8BeginEl, ui8EndEl)) {
        sendRequest (reqInfo, ui32MsgSeqId, ui8BeginEl, ui8EndEl, pMessageRequests);
    }

    DisServiceDataReqMsg *pDDRM = new DisServiceDataReqMsg (_pDisService->getNodeId(), NULL, // pszQueryTargetNodeId
                                                            _pPeerState->getNumberOfActiveNeighbors(),
                                                            pMessageRequests, NULL);
    int rc = _pDisService->broadcastDisServiceCntrlMsg (pDDRM, NULL, "Sending DisServiceDataReqMsg", NULL, NULL);
    _pDisService->getStats()->missingFragmentRequestSent (pDDRM->getSize());

    delete pMessageRequests;

    _m.unlock (265);
    return rc;
}

int MessageReassembler::sendRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                                     uint8 ui8BeginEl, uint8 ui8EndEl,
                                     PtrLList<FragmentRequest> *pMessageRequests)
{
    _m.lock (266);
    int rc = sendRequestInternal (reqInfo, ui32MsgSeqId, ui8BeginEl, ui8EndEl, pMessageRequests);
    _m.unlock (266);
    return rc;
}

int MessageReassembler::sendRequestInternal (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                                             uint8 ui8BeginEl, uint8 ui8EndEl,
                                             PtrLList<FragmentRequest> *pMessageRequests)
{
    if (reqInfo._pszGroupName == NULL || reqInfo._pszSenderNodeId == NULL || pMessageRequests == NULL) {
        return -1;
    }

    for (uint8 ui8ChunkId = ui8BeginEl; ui8ChunkId <= ui8EndEl; ui8ChunkId++) {
        if (!containsMessage (reqInfo._pszGroupName, reqInfo._pszSenderNodeId, ui32MsgSeqId, ui8ChunkId)) {
            // create and add the request
            checkAndLogMsg ("MessageReassembler::sendRequest", Logger::L_LowDetailDebug,
                            "requesting %s:%s:%u:%u\n", reqInfo._pszGroupName, reqInfo._pszSenderNodeId, ui32MsgSeqId, ui8ChunkId);
            FragmentRequest *pFragReq = new DisServiceDataReqMsg::ChunkMessageRequest (reqInfo._pszGroupName, reqInfo._pszSenderNodeId, ui32MsgSeqId, ui8ChunkId);
            if (pFragReq == NULL) {
                checkAndLogMsg ("MessageReassembler::sendRequest", memoryExhausted);
                break;
            }
            pMessageRequests->prepend (pFragReq);
        }
    }

    return 0;
}

bool MessageReassembler::getAndResetNewPeer()
{
    _newPeerMutex.lock (182);
    bool bRet = _bNewPeer;
    _bNewPeer = false;
    _newPeerMutex.unlock (182);
    return bRet;
}

void MessageReassembler::run (void)
{
    const char *pszMethodName = "MessageReassembler::run";
    setName (pszMethodName);

    started();
    int64 i64SleepTime, i64Random;
    char **ppszInterfaces;
    while (!terminationRequested()) {
        i64SleepTime = _pDisService->getMissingFragmentTimeout();
        // Compute missing fragment timeout's 20% random delay/anticipation
        i64Random = i64SleepTime / 5;
        i64Random = (rand() % i64Random);
        if (((rand() % 100) + 1.0f) < 50.0f) {
            i64SleepTime -= i64Random;
        }
        else {
            i64SleepTime += i64Random;
        }
        sleepForMilliseconds (i64SleepTime);

        if ((_pTrSvc != NULL) && (_pTrSvc->getIncomingQueueSize() < _ui32IncomingQueueSizeRequestThreshold)) {
            /*
            if (_pTrSvc->getAsyncTransmission()) {
                pszInterfaces = _pTrSvc->getActiveInterfacesAddress();
            }
            else {
                pszInterfaces = _pTrSvc->getInterfacesByReceiveRate (_fReceiveRateThreshold);
            }*/
            //THE CHECK ON THE RECEIVE RATE THRESHOLD SHOULD BE REMOVED.
            //UNNECESSARY WITH PROPER BANDWIDTH SHARING ON THE DATA REQUEST HANDLER
            ppszInterfaces = _pTrSvc->getActiveInterfacesAddress();
            if (ppszInterfaces != NULL) {
                _m.lock (183);

                // Determine the maximum size of the Missing Fragment Request message
                uint16 ui16MaxMsgSize = minimum (_pTrSvc->getMTU(), _pDisService->getMaxFragmentSize());

                // Add missing fragment requests
                fillMessageRequestScheduler();

                // Add missing message requests
                _pSubState->getMissingMessageRequests (&_requestSched, this);
                _requestState.getMissingMessageRequests (&_requestSched, this);
                loadOpportunisticallyCachedFragments();

                while (!_requestSched.isEmpty() && (ui16MaxMsgSize > 0)) {
                    DisServiceIncrementalDataReqMsg dataReq (_pDisService->getNodeId(),
                                                             NULL, // pszQueryTargetNodeId
                                                             _pPeerState->getNumberOfActiveNeighbors(),
                                                             &_requestSched, ui16MaxMsgSize);

                    checkAndLogMsg (pszMethodName, Logger::L_Info, "++++++++++++++++Sending Fragment "
                                    "Request (ui16MaxMsgSize <%u>) - \n", ui16MaxMsgSize);
                    _pDisService->broadcastDisServiceCntrlMsg (&dataReq, (const char**) ppszInterfaces, "Sending DisServiceDataReqMsg");
                    _pDisService->getStats()->missingFragmentRequestSent (dataReq.getSize());

                    // Reset the scheduler _before_ deleting the lists
                    //_requestSched.reset();
                }

                _m.unlock (183);

                if (ppszInterfaces) {
                    for (int i = 0; ppszInterfaces[i] != NULL; i++) {
                        free (ppszInterfaces[i]);
                    }
                    free (ppszInterfaces);
                }
            }
        }
    }
    terminating();
}

//------------------------------------------------------------------------------
// PRIVATE METHODS
//------------------------------------------------------------------------------

bool MessageReassembler::addingRangeRequired (FragmentedMessage *pFragMsg)
{
    return pFragMsg->bReliabilityRequired;
}

int MessageReassembler::deliverCompleteMessage (MessageHeader *pMH, void *pData, uint32 ui32DataLength,
                                                MessagesByPublisher *pMS, FragmentedMessage *pFragMsg,
                                                bool bIsMetaDataPart, bool bIsNotTarget)
{
    const char *pszMethodName = "MessageReassembler::deliverCompleteMessage";
    MessageHeader *pMHClone = pMH->clone();
    bool bIsChunk = pMHClone->isChunk();

    // Sanity checks
    if (bIsMetaDataPart && bIsChunk) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                       "A Chunk can not have a metadata part. Error in %s\n",
                       pMHClone->getMsgSeqId());
        delete pMHClone;
        free (pData);
        pMHClone = NULL;
        pData = NULL;
        return -1;
    }
    if (!checkCompleteMessageLength (pMHClone, ui32DataLength, bIsChunk, bIsMetaDataPart)) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Delivering an incomplete Message. TotalMsgLength <%d> MetaDataLength <%d> FragmentLength <%d>\n",
                        pMHClone->getTotalMessageLength(),
                        (bIsChunk ? 0 : ((MessageInfo*)pMHClone)->getMetaDataLength()),
                        ui32DataLength);
        delete pMHClone;
        free (pData);
        pMHClone = NULL;
        pData = NULL;
        return -1;
    }

    // Status update and caching
    bool bFreeUpMemory = false;
    pMHClone->setFragmentOffset (0);
    if (bIsMetaDataPart) {
        // metadata
        pMHClone->setFragmentLength (((MessageInfo*)pMHClone)->getMetaDataLength());
        pFragMsg->bMetaDataDelivered = true;
    }
    else {
        // complete data or complete chunk
        pMHClone->setFragmentLength (pMHClone->getTotalMessageLength());
        // store the complete data in the cache
        // NOTE: it is assumed that cached data is copied!
        _pDisService->addDataToCache (pMHClone, pData);
        bFreeUpMemory = true;
    }

    Message *pMsg = new Message (pMHClone, pData);

    // The message must always be removed from _pReqState
    RequestDetails *pDetails = _requestState.messageArrived (pMsg);
    if (pFragMsg->pRequestDetails != NULL) {
        RequestDetails::QueryDetails *pCurr = pFragMsg->pRequestDetails->_details.getFirst();
        if ((pDetails == NULL) && (pCurr != NULL)) {
            pDetails = new RequestDetails (pCurr->_queryId, pCurr->_ui16ClientId, pCurr->_i64ExpirationTime);
            pCurr = pFragMsg->pRequestDetails->_details.getNext();
        }
        while (pCurr != NULL) {
            pDetails->addDetails (pCurr->_queryId, pCurr->_ui16ClientId, pCurr->_i64ExpirationTime);
            pCurr = pFragMsg->pRequestDetails->_details.getNext();
        }
    }

    // Deliver the message to SubscriptionState
    if (bIsNotTarget) {
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                        "Reassembled message %s, but the node is not the target. "
                        "It can't be delivered to the application.\n",
                        pMsg->getMessageHeader()->getMsgId());
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                        "calling messageArrived on SubscriptionState for message id <%s>\n",
                        pMsg->getMessageInfo()->getMsgId());
        _pSubState->messageArrived (pMsg, pFragMsg->bIsHistoryMsg, pDetails);
    }

    if (pDetails != NULL) {
        free (pDetails);
        pDetails = NULL;
    }

    _pDisService->getStats()->dataMessageReceived (pMH->getPublisherNodeId(), pMH->getGroupName(),
                                                   pMH->getTag(), pMH->getTotalMessageLength());

    // Delete the Fragmented Message and all the Fragments
    if (bFreeUpMemory) {
        // remove the fragmented message
        pMS->messages.remove (pFragMsg);
        if (pFragMsg != NULL) {
            delete pFragMsg->pMH;
            pFragMsg->pMH = NULL;
            delete pFragMsg;
            pFragMsg = NULL;
        }
    }

    // pMHClone and pData should not be deallocated
    return 0;
}

void MessageReassembler::fillMessageRequestScheduler()
{
    int64 i64CurrentTime = getTimeInMilliseconds();

    // Get the uncompleted message's missing fragments
    bool bNewPeerArrived = getAndResetNewPeer();
    PtrLList<FragmentedMessage> *pNoLongerRelevantMessages = NULL;
    for (StringHashtable<MessagesByGroup>::Iterator i = _receivedMessages.getAllElements(); !i.end(); i.nextElement()) {
        MessagesByGroup *pMG = i.getValue();
        for (StringHashtable<MessagesByPublisher>::Iterator iGroup = pMG->messagesInGroup.getAllElements(); !iGroup.end(); iGroup.nextElement()) {
            MessagesByPublisher *pMP = iGroup.getValue();
            for (FragmentedMessage *pFragMsg = pMP->messages.getFirst(); pFragMsg != NULL; pFragMsg = pMP->messages.getNext()) {
                if (!_pSubState->isRelevant (pFragMsg->pMH->getGroupName(), pFragMsg->pMH->getPublisherNodeId(),
                                             pFragMsg->pMH->getMsgSeqId(), pFragMsg->pMH->getChunkId())) {
                    if (pNoLongerRelevantMessages == NULL) {
                        pNoLongerRelevantMessages = new PtrLList<FragmentedMessage>();
                    }
                    if (pNoLongerRelevantMessages != NULL) {
                        pNoLongerRelevantMessages->prepend (pFragMsg);
                    }
                    continue;
                }
                if (!pFragMsg->bReliabilityRequired) {
                    // The missing fragment's list of this FragmentedMessage
                    // should not be considered since no one client subscribing
                    // this FragmentedMessage's group used the flag bReliable
                    continue;
                }

                if (_iMaxNumberOfReqs != UNLIMITED_MAX_NUMBER_OF_REQUESTS &&
                    pFragMsg->iRequested > _iMaxNumberOfReqs) {
                    // This message has been requested too many times. Stop
                    // requesting it
                    continue;
                }

                int64 i64TimeOut = getMissingFragmentRequestTimeOut (i64CurrentTime, pFragMsg->i64LastNewDataArrivalTime);
                if (_bExpBackoff && !bNewPeerArrived &&
                    (pFragMsg->i64LastMissingFragmentsRequestTime > 0) && // if i64LastMissingFragmentsRequestTime is
                                                                          // 0, fragments have not been requested yet
                                                                          // thus the request must be performed
                    i64CurrentTime < (pFragMsg->i64LastMissingFragmentsRequestTime + i64TimeOut)) {
                    // Too soon to request missing fragments for this message - skip it for now
                    continue;
                }

                addToRequestScheduler (pFragMsg, i64CurrentTime);
            }
        }
    }

    if (pNoLongerRelevantMessages != NULL) {
        FragmentedMessage *pFragMsg;
        pNoLongerRelevantMessages->resetGet();
        while (NULL != (pFragMsg = pNoLongerRelevantMessages->getNext())) {
            MessagesByGroup *pMG = _receivedMessages.get (pFragMsg->pMH->getGroupName());
            if (pMG == NULL) {
                checkAndLogMsg ("MessageReassembler::fillMessageRequestScheduler", Logger::L_Warning,
                                "could not find group <%s> while deleting message <%s> that is no longer relevant\n",
                                pFragMsg->pMH->getGroupName(), pFragMsg->pMH->getMsgId());
            }
            else {
                MessagesByPublisher *pMP = pMG->messagesInGroup.get (pFragMsg->pMH->getPublisherNodeId());
                if (pMP == NULL) {
                    checkAndLogMsg ("MessageReassembler::fillMessageRequestScheduler", Logger::L_Warning,
                                    "could not find messages by publisher <%s> while deleting message <%s> that is no longer relevant\n",
                                    pFragMsg->pMH->getPublisherNodeId(), pFragMsg->pMH->getMsgId());
                }
                else {
                    if (NULL == pMP->messages.remove (pFragMsg)) {
                        checkAndLogMsg ("MessageReassembler::fillMessageRequestScheduler", Logger::L_Warning,
                                        "failed to delete message <%s> that is no longer relevant\n", pFragMsg->pMH->getMsgId());
                    }
                    else {
                        checkAndLogMsg ("MessageReassembler::fillMessageRequestScheduler", Logger::L_LowDetailDebug,
                                        "deleted message <%s> that is no longer relevant\n", pFragMsg->pMH->getMsgId());
                        delete pFragMsg->pMH;
                        pFragMsg->pMH = NULL;
                        delete pFragMsg;
                        pFragMsg = NULL;
                    }
                }
            }
        }
        delete pNoLongerRelevantMessages;
    }
}

int MessageReassembler::addToRequestScheduler (FragmentedMessage *pFragMsg, int64 i64CurrentTime)
{
    updateMissingFragmentsList (pFragMsg);

    if ((i64CurrentTime - pFragMsg->i64LastNewDataArrivalTime) < _pDisService->getMissingTailFragmentTimeout()) {
        if (pFragMsg->missingFragments.getFirst() != NULL) {
            _requestSched.addRequest (new FragmentRequest (pFragMsg->pMH, &(pFragMsg->missingFragments)),
                                      pFragMsg->bSequentialTransmission, pFragMsg->bReliabilityRequired);
            pFragMsg->i64LastMissingFragmentsRequestTime = i64CurrentTime;
            pFragMsg->iRequested = pFragMsg->iRequested + 1;
        }
    }
    else {
        uint32 ui32TailOffset = pFragMsg->ui32NextExpected;
        uint32 ui32TailEnd = pFragMsg->pMH->getTotalMessageLength();
        if (ui32TailOffset < ui32TailEnd) {
            _requestSched.addRequest (new FragmentRequest (pFragMsg->pMH, &(pFragMsg->missingFragments),
                                                           ui32TailOffset, ui32TailEnd),
                                                           pFragMsg->bSequentialTransmission,
                                                           pFragMsg->bReliabilityRequired);
            pFragMsg->i64LastMissingFragmentsRequestTime = i64CurrentTime;
            pFragMsg->iRequested = pFragMsg->iRequested + 1;
        }
        else if (pFragMsg->missingFragments.getFirst() != NULL) {
            _requestSched.addRequest (new FragmentRequest (pFragMsg->pMH, &(pFragMsg->missingFragments)),
                                      pFragMsg->bSequentialTransmission, pFragMsg->bReliabilityRequired);
            pFragMsg->i64LastMissingFragmentsRequestTime = i64CurrentTime;
            pFragMsg->iRequested = pFragMsg->iRequested + 1;
        }
    }

    return 0;
}

int64 MessageReassembler::getMissingFragmentRequestTimeOut (int64 i64CurrentTime, int64 i64FragmentLastNewDataArrivalTime)
{
    if (!_bExpBackoff) {
        return 0;
    }
    int64 iElapsedTimeSinceLastNewData = i64CurrentTime - i64FragmentLastNewDataArrivalTime;
    uint32 ui32ElapsedTimeFactor = (uint32) (iElapsedTimeSinceLastNewData / _pDisService->getMissingFragmentTimeout());
    if (ui32ElapsedTimeFactor < 3) {
        return _pDisService->getMissingFragmentTimeout();
    }
    else {
        // Exponential backoff
        ui32ElapsedTimeFactor -= 3;
        return ui32ElapsedTimeFactor * ui32ElapsedTimeFactor * _pDisService->getMissingFragmentTimeout();
    }
}

void MessageReassembler::loadOpportunisticallyCachedFragments (void)
{
    const char *pszMethodName = "MessageReassembler::loadOpportunisticallyCachedFragments";

    PtrLList<MessageRequestScheduler::FragReqWrapper> *pRequests = _requestSched.getRequests();
    if (pRequests == NULL) {
        return;
    }
    for (MessageRequestScheduler::FragReqWrapper *pFragWr = pRequests->getFirst(); pFragWr != NULL; pFragWr = pRequests->getNext()) {
        if ((pFragWr->pFRequests != NULL) && (pFragWr->pFRequests->pRequestedRanges != NULL) && (pFragWr->pFRequests->pMsgHeader != NULL)) {
            DisServiceMsg::Range *pRange = pFragWr->pFRequests->pRequestedRanges->getFirst();
            // if it's a complete message request
            if ((pRange != NULL) && (pRange->getFrom() == 0) && (pRange->getTo() == 0)) {
                // load opportunistically cached fragments
                MessageHeader *pMH = pFragWr->pFRequests->pMsgHeader;
                checkAndLogMsg (pszMethodName, Logger::L_Info, "loading opportunistically cached fragments for %s.\n", pMH->getMsgId());
                if (MessageReassemblerUtils::loadOpportunisticallyCachedFragments (this, _pDisService->_pDataCacheInterface, pMH, false)) {
                    checkAndLogMsg (pszMethodName, Logger::L_Info, "removing request for %s fragments (1)\n", pMH->getMsgId());
                    _requestSched.removeRequest (pFragWr);
                }
                else if (isBeingReassembledInternal (pMH->getGroupName(), pMH->getPublisherNodeId(), pMH->getMsgSeqId(), pMH->getChunkId(), false)) {
                    checkAndLogMsg (pszMethodName, Logger::L_Info, "removing request for %s fragments (2)\n", pMH->getMsgId());
                    _requestSched.removeRequest (pFragWr);
                }
                else {
                    checkAndLogMsg (pszMethodName, Logger::L_Info, "no fragments were opportunistically cached for %s.\n", pMH->getMsgId());
                }
            }
        }
    }
}

uint8 MessageReassembler::messageComplete (FragmentedMessage *pFragMsg)
{
    const char *pszMethodName = "MessageReassembler::messageComplete";

    if (pFragMsg->pMH->getTotalMessageLength() == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,  "Trying to reassemble dummy message!\n");
        return DUMMY_MSG;
    }

    // Check whether the message, or its metadata part, could be complete (fragments
    // may overlap, therefore I have to check for contiguity to make sure)
    const uint32 ui32MetadataLen = pFragMsg->pMH->isChunk() ? 0U : ((MessageInfo*)pFragMsg->pMH)->getMetaDataLength();
    if ((ui32MetadataLen > 0) && (!pFragMsg->bMetaDataDelivered)) {
       if (pFragMsg->getCachedBytes() < ui32MetadataLen) {
           return UNCOMPLETE;
       }
    }
    else if (pFragMsg->getCachedBytes() < pFragMsg->pMH->getTotalMessageLength()) {
        return UNCOMPLETE;
    }

    uint32 ui32NextExpected = 0;
    uint32 ui32MessageLength = 0;
    int64 i64Diff;
    uint32 ui32NewNextExpected;
    for (FragmentWrapper *pFragWrapper = pFragMsg->getFirstFragment(); NULL != pFragWrapper; pFragWrapper = pFragMsg->getNextFragment()) {
        i64Diff = (int64)ui32NextExpected - (int64)pFragWrapper->ui32FragmentOffset;
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "NextExpected <%u> Offset <%u> "
                        "Length <%u>\n", ui32NextExpected, pFragWrapper->ui32FragmentOffset,
                        pFragWrapper->ui16FragmentLength);
        if (i64Diff < 0) {
            // missing fragment!
            break;
        }

        // Update next expected
        ui32NewNextExpected = pFragWrapper->ui32FragmentOffset + pFragWrapper->ui16FragmentLength;
        if (ui32NewNextExpected > ui32NextExpected) {
            ui32MessageLength += minimum (ui32NewNextExpected - ui32NextExpected, (uint32)pFragWrapper->ui16FragmentLength);
            ui32NextExpected = ui32NewNextExpected;
        }
    }

    if (ui32MessageLength < pFragMsg->pMH->getTotalMessageLength()) {
        if (!pFragMsg->pMH->isChunk()) {
            // The message is not complete, but the metadata part may be.
            // NOTE: this option is not available for Chunk messages.
            if ((ui32MetadataLen > 0) && (ui32MessageLength >= ui32MetadataLen)) {
                // There is a metadata and ui32MessageLength is enough bytes to
                // reassemble the metadata.
                // Check if there are missing fragments in range [0, ui32MetadataLen]
                if (ui32NewNextExpected >= ui32MetadataLen) {
                    // The first missing fragments has offset > than the length
                    // of ui32MetadataLen, thus the metadata is complete
                    // Checking only the first message is enough, since missingFragments
                    // orders the fragments is in ascending order.
                    return METADATA_COMPLETE;
                }
            }
        }
        // There is not a metadata, or it has not been received a number of byte
        // enough to reassemble the metadata
        return UNCOMPLETE;
    }
    return MSG_COMPLETE;
}

void * MessageReassembler::reassemble (FragmentedMessage *pFragMsg, bool bReassembleOnlyMetaDataPart, uint32 &ui32BytesWritten)
{
    if (bReassembleOnlyMetaDataPart) {
        if (pFragMsg->pMH->isChunk()) {
            checkAndLogMsg ("MessageReassembler::reassemble", Logger::L_SevereError,
                            "Cannot reassemble metadata part in chunk");
            ui32BytesWritten = 0;
            return NULL;
        }
        if (((FragmentedMessage*)pFragMsg)->bMetaDataDelivered) {
            checkAndLogMsg ("MessageReassembler::reassemble", Logger::L_SevereError,
                            "The metadata has already been reassembled");
            ui32BytesWritten = 0;
            return NULL;
        }
    }

    // Assume we have all the fragments - reassemble
    uint32 ui32NextExpected = 0;

    // create a buffer of the proper size
    uint32 ui32TotMsgLen;
    if (bReassembleOnlyMetaDataPart) {
        ui32TotMsgLen = ((MessageInfo*)(pFragMsg->pMH))->getMetaDataLength();
    }
    else {
        ui32TotMsgLen = pFragMsg->pMH->getTotalMessageLength();
    }
    char *pMsgBuf = (char*) calloc (ui32TotMsgLen, sizeof(char));

    for (FragmentWrapper *pFragWrapper = pFragMsg->getFirstFragment(); (NULL !=  pFragWrapper) &&
            (ui32NextExpected < ui32TotMsgLen); pFragWrapper = pFragMsg->getNextFragment()) {
        uint32 ui32Overlap = 0;
        if (ui32NextExpected > pFragWrapper->ui32FragmentOffset) {
            ui32Overlap = ui32NextExpected - pFragWrapper->ui32FragmentOffset;
            if (ui32Overlap > pFragWrapper->ui16FragmentLength) {
                continue;
            }
        }
        if (pFragWrapper->ui32FragmentOffset > ui32NextExpected) {
            checkAndLogMsg ("MessageReassembler::reassemble", Logger::L_SevereError,
                            "Missing fragment, the message can not be reassembled. Offset: <%u> NextExpected: <%u> TotMsgLen: <%u> %s\n",
                            pFragWrapper->ui32FragmentOffset, ui32NextExpected, ui32TotMsgLen,
                            (bReassembleOnlyMetaDataPart) ? "(metadata only)" : "");
            free (pMsgBuf);
            pMsgBuf = NULL;
            ui32BytesWritten = 0;
            return NULL;
        }

        int64 i64Diff = (int64)pFragWrapper->ui32FragmentOffset + (int64)pFragWrapper->ui16FragmentLength - (int64)ui32TotMsgLen;
        if (i64Diff < 0) {
            // No need to trunk the last fragment
            i64Diff = 0;
        }
        uint32 ui32FragLen = pFragWrapper->ui16FragmentLength - ui32Overlap - ((uint32)i64Diff);
        if (ui32FragLen > 0) {
            memcpy (pMsgBuf+ui32NextExpected, (char*)(pFragWrapper->pFragment)+ui32Overlap, ui32FragLen);
            ui32NextExpected += ui32FragLen;
        }
    }
    ui32BytesWritten = ui32NextExpected;
    return pMsgBuf;
}

int MessageReassembler::updateMissingFragmentsList (FragmentedMessage *pFMH)
{
    const char *pszMethodName = "MessageReassembler::updateMissingFragmentsList";
    if (!pFMH->bReliabilityRequired) {
        // no one client subscribing this FragmentedMessage's group used the
        // flag bReliable there is no need to fill the missing fragment's list
        return 0;
    }

    uint32 ui32NextExpected = 0;
    uint32 ui32MessageLength = 0;
    pFMH->resetMissingFragmentsList();
    for (FragmentWrapper *pFragWrapper = pFMH->getFirstFragment(); NULL != pFragWrapper; pFragWrapper = pFMH->getNextFragment()) {
        if ((pFragWrapper->ui32FragmentOffset > ui32NextExpected) && addingRangeRequired (pFMH)) {
            pFMH->missingFragments.append (new Range (ui32NextExpected, pFragWrapper->ui32FragmentOffset));
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Missing fragment range <%u-%u> added to missingFragment list.\n",
                            ui32NextExpected, pFragWrapper->ui32FragmentOffset);
        }
        ui32NextExpected = pFragWrapper->ui32FragmentOffset + pFragWrapper->ui16FragmentLength;
        ui32MessageLength += pFragWrapper->ui16FragmentLength;
    }

    return 0;
}

//==============================================================================
// FragmentedMessage
//==============================================================================

MessageReassembler::FragmentedMessage::FragmentedMessage (MessageHeader *pMI, bool isReliable, bool isSequenced, bool bIsInHistory, bool bNotTarget)
    : pMH (pMI->clone()),
      ui32NextExpected (pMI->getFragmentOffset() + pMI->getFragmentLength()),
      i64LastNewDataArrivalTime (0),
      i64LastMissingFragmentsRequestTime (0),
      ui32MsgSeqId (pMI->getMsgSeqId()),
      ui8ChunkID (pMI->getChunkId()),
      bReliabilityRequired (isReliable),
      bSequentialTransmission (isSequenced),
      bMetaDataDelivered (false),
      bIsHistoryMsg (bIsInHistory),
      iRequested (0),
      bIsNotTarget (bNotTarget),
      pRequestDetails (NULL),
      ui64CachedBytes (0U),
      fragments (false)
{
    pMH->setFragmentLength (0);    // Reset FragmentLength and FragmentOffset since we are
    pMH->setFragmentOffset (0);    // storing this MessageInfo for all the fragments
}

MessageReassembler::FragmentedMessage::~FragmentedMessage (void)
{
    FragmentWrapper *pFragWr, *pFragWrTmp;
    pFragWrTmp = fragments.getFirst();
    while ((pFragWr = pFragWrTmp) != NULL) {
        pFragWrTmp = fragments.getNext();
        fragments.remove (pFragWr);
        free (pFragWr->pFragment);
        pFragWr->pFragment = NULL;
        delete pFragWr;
    }

    resetMissingFragmentsList();
}

void MessageReassembler::FragmentedMessage::resetMissingFragmentsList (void)
{
    Range *pRange, *pRangeTmp;
    pRangeTmp = missingFragments.getFirst();
    while ((pRange = pRangeTmp)) {
        pRangeTmp = missingFragments.getNext();
        missingFragments.remove (pRange);
        delete pRange;
    }
}

void MessageReassembler::FragmentedMessage::addFragment (FragmentWrapper *pFragmentWrap)
{
    i64LastNewDataArrivalTime = getTimeInMilliseconds();
    ui64CachedBytes += pFragmentWrap->ui16FragmentLength;
    fragments.insert (pFragmentWrap);
}

uint64 MessageReassembler::FragmentedMessage::getCachedBytes (void) const
{
    return ui64CachedBytes;
}

MessageReassembler::FragmentWrapper * MessageReassembler::FragmentedMessage::getFirstFragment (void)
{
    return fragments.getFirst();
}

MessageReassembler::FragmentWrapper * MessageReassembler::FragmentedMessage::getNextFragment (void)
{
    return fragments.getNext();
}
            
MessageReassembler::FragmentWrapper * MessageReassembler::FragmentedMessage::searchFragment (FragmentWrapper &fragWrap)
{
    return fragments.search (&fragWrap);
}
                
void MessageReassembler::FragmentedMessage::updateNextExpectedValue (void)
{
    ui32NextExpected = 0;
    FragmentWrapper *pWrapper = fragments.getFirst();
    while (pWrapper != NULL) {
        if ((ui32NextExpected >= pWrapper->ui32FragmentOffset) && (ui32NextExpected <= pWrapper->ui32FragmentOffset + pWrapper->ui16FragmentLength)) {
            ui32NextExpected = pWrapper->ui32FragmentOffset + pWrapper->ui16FragmentLength;
        }
        else if (ui32NextExpected < pWrapper->ui32FragmentOffset) {
            break;
        }
        pWrapper = fragments.getNext();
    }
}

MessageReassembler::FragmentedMessage::FragmentedMessage (uint32 ui32MsgSeqId, uint8 ui8ChunkID)
    : pMH (NULL),
      ui32NextExpected (0),
      i64LastNewDataArrivalTime (0),
      i64LastMissingFragmentsRequestTime (0),
      ui32MsgSeqId (ui32MsgSeqId),
      ui8ChunkID (ui8ChunkID),
      bReliabilityRequired (false),
      bSequentialTransmission (false),
      bMetaDataDelivered (false),
      bIsHistoryMsg (false),
      iRequested (0),
      bIsNotTarget (false),
      pRequestDetails (NULL),
      ui64CachedBytes (0U),
      fragments (false)
{
}

//==============================================================================
// MessagesBySender
//==============================================================================

MessageReassembler::MessagesByPublisher::~MessagesByPublisher()
{
    FragmentedMessage *pFragMsg, *pFragMsgTmp;
    pFragMsgTmp = messages.getFirst();
    while ((pFragMsg = pFragMsgTmp) != NULL) {
        pFragMsgTmp = messages.getNext();
        messages.remove (pFragMsg);
        delete pFragMsg;
    }
    pFragMsg = NULL;
}

//==============================================================================
// MessagesByGroup
//==============================================================================

MessageReassembler::MessagesByGroup::~MessagesByGroup()
{
}

////////////////////////////////////////////////////////////////////////////////

bool checkCompleteMessageLength (MessageHeader *pMI, uint32 ui32DataLength, bool bIsChunk, bool bIsMetaDataPart)
{
    if (bIsMetaDataPart) {
        // It's complete metadata part
        if (ui32DataLength == ((MessageInfo*)pMI)->getMetaDataLength()) {
            return true;
        }
    }
    return (ui32DataLength == pMI->getTotalMessageLength());
}

bool MessageReassemblerUtils::loadOpportunisticallyCachedFragments (MessageReassembler *pMsgReassembler, DataCacheInterface *pDCI,
                                                                    MessageHeader *pMH, bool bNotTarget)
{
    if (pMH == NULL) {
        return false;
    }
    MessageId msgId (pMH->getGroupName(), pMH->getPublisherNodeId(), pMH->getMsgSeqId(), pMH->getChunkId());
    return loadOpportunisticallyCachedFragmentsInternal (&msgId, pMsgReassembler, pDCI, pMH, bNotTarget);
}

bool MessageReassemblerUtils::loadAllOpportunisticallyCachedFragments (MessageId *pMsgId, MessageReassembler *pMsgReassembler,
                                                                       DataCacheInterface *pDCI)
{
    return loadOpportunisticallyCachedFragmentsInternal (pMsgId, pMsgReassembler, pDCI, NULL, false);
}

bool MessageReassemblerUtils::loadOpportunisticallyCachedFragmentsInternal (MessageId *pMsgId, MessageReassembler *pMsgReassembler,
                                                                            DataCacheInterface *pDCI, MessageHeader *pMHFilter, bool bNotTarget)
{
    if (pMsgId == NULL || pMsgReassembler == NULL || pDCI == NULL) {
        return false;
    }

    if (pMsgReassembler->containsMessage (pMsgId)) {
        // If an entry for the message already exists, it means that the
        // opportunistically cached fragments have already been loaded
        return false;
    }

    // There is no entry for the message - load what is already in the cache first
    
    PtrLList<Message> *pMessages = pDCI->getMessages (pMsgId->getGroupName(), pMsgId->getOriginatorNodeId(),
                                                      0, pMsgId->getSeqId(), pMsgId->getSeqId());
    if (pMessages == NULL) {
        checkAndLogMsg ("MessageReassemblerUtils::loadOpportunisticallyCachedFragmentsInternal", Logger::L_Info,
                        "no fragments for message <%s>\n", pMsgId->getId());
        return 0;
    }

    uint64 ui64LoadedBytes = 0U;
    bool bAtLeastOneInsert = false;
    for (Message *pCurrMsg = pMessages->getFirst(); pCurrMsg != NULL; pCurrMsg = pMessages->getNext()) {
        uint32 ui32Len = pCurrMsg->getMessageHeader()->getFragmentLength();
        if ((pMHFilter == NULL) ||
            (pCurrMsg->getMessageHeader()->getFragmentOffset() != pMHFilter->getFragmentOffset() ||
                (ui32Len != pMHFilter->getFragmentLength()))) {
            // This message retrieved from the database is not the same as the
            // one just received over the network - process it

            // It is necessary to make a copy of pData, because MessageReassembler
            // does not make a copy of it, and it deallocates the memory allocated
            // for a fragment when the whole message is complete
            Message msgToAdd;
            void *pData = malloc (ui32Len);
            if (pData != NULL) {
                memcpy (pData, pCurrMsg->getData(), ui32Len);
                msgToAdd.setData (pData);
                msgToAdd.setMessageHeader (pCurrMsg->getMessageHeader());
                if (0 == pMsgReassembler->fragmentArrived (&msgToAdd, bNotTarget)) {
                    bAtLeastOneInsert = true;
                    ui64LoadedBytes += msgToAdd.getMessageHeader()->getFragmentLength();
                }
            }
        }
    }

    checkAndLogMsg ("MessageReassemblerUtils::loadOpportunisticallyCachedFragmentsInternal", Logger::L_Info,
                    "loaded %llu bytes of fragments for message <%s> (there may be overlapping fragments)\n",
                    ui64LoadedBytes, pMsgId->getId());

    if ((bAtLeastOneInsert) && (!pMsgReassembler->containsMessage (pMsgId))) {
        // The message has been reassembled
        pDCI->release (pMessages);
        return true;
    }
    pDCI->release (pMessages);

    return false;
}

