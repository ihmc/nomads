/*
 * ChunkRetrievalController.cpp
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

#include "ChunkRetrievalController.h"

#include "DataCache.h"
#include "DataCacheInterface.h"
#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "DisServiceMsg.h"
#include "DSSFLib.h"
#include "MessageReassembler.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

char **createOutgoingInterfacesList (const char *pszOutgoingInterface)
{
    char **ppszOutogoingInterfaces = (char **) calloc (2, sizeof (char*));
    if (ppszOutogoingInterfaces != NULL) {
        ppszOutogoingInterfaces[0] = strDup (pszOutgoingInterface);
    }
    return ppszOutogoingInterfaces;
}

ChunkRetrievalController::ChunkRetrievalController (DisseminationService *pDisService,
                                                    ChunkDiscoveryController *pDiscoveryCtrl,
                                                    DataCacheInterface *pDataCacheInterface)
    : MessagingService (pDisService),
      _hitHistoryTable (true, // bCaseSensitiveKeys
                        true, // bCloneKeys
                        true, // bDeleteKeys
                        true) // bDeleteValues
{
    _pDisService = pDisService;
    _pDiscoveryCtrl = pDiscoveryCtrl;
    _pDataCacheInterface = pDataCacheInterface;    
    if (_pDisService == NULL || _pDiscoveryCtrl == NULL ||
        _pDataCacheInterface == NULL) {
        checkAndLogMsg ("ChunkRetrievalController::ChunkRetrievalController",
                        Logger::L_SevereError, "could not initialize ChunkRetrievalController\n");
    }
    _pHit = NULL;
    _pListHits = NULL;
    _pContainerHitsList = NULL;
}

ChunkRetrievalController::~ChunkRetrievalController()
{
   for (StringHashtable<ContainerHitsList>::Iterator it = _hitHistoryTable.getAllElements();
        !(it.end()); it.nextElement()) {

	_pContainerHitsList = it.getValue();
        _pListHits = _pContainerHitsList->_pListHits;
        for (_pHit = _pListHits->getFirst(); _pHit != NULL; _pHit = _pListHits->getNext()) {
            delete (_pHit);
        }
        delete _pListHits; _pListHits = NULL; 
        delete _pContainerHitsList; _pContainerHitsList = NULL;
    }
}

/**
 * It checks which type of message is arrived and respond it.
 */
void ChunkRetrievalController::newIncomingMessage (const void *, uint16, DisServiceMsg *pDisServiceMsg,
                                                   uint32, const char *pszIncomingInterface) 
{
    if (pDisServiceMsg == NULL) {
        checkAndLogMsg ("ChunkRetrievalController::newIncomingMessage",
                        Logger::L_SevereError, "pDisServiceMsg is null\n");
        return;
    }

    int rc = 0;
    switch (pDisServiceMsg->getType())
    {
        case DisServiceCtrlMsg::CRMT_Query:
        {
            ChunkRetrievalMsgQuery *pCRMQ = (ChunkRetrievalMsgQuery*) pDisServiceMsg;
            checkAndLogMsg ("ChunkRetrievalController::newIncomingMessage", Logger::L_LowDetailDebug,
                            "received a query from %s; query id is %s\n",
                            pCRMQ->getSenderNodeId(), pCRMQ->getQueryId());
            rc = replyToQuery (pCRMQ, pszIncomingInterface); 
            break;
        }

        case DisServiceCtrlMsg::CRMT_QueryHits:
        {
            ChunkRetrievalMsgQueryHits *pCRMQH = (ChunkRetrievalMsgQueryHits*) pDisServiceMsg;
            checkAndLogMsg ("ChunkRetrievalController::newIncomingMessage", Logger::L_LowDetailDebug,
                            "received a query hit for %s from %s\n", pCRMQH->getQueryId(), pCRMQH->getSenderNodeId());
            rc = replyToQueryHits (pCRMQH);
            break;
        }

        default:
            break;
    }

    if (rc < 0) {
        checkAndLogMsg ("ChunkRetrievalController::newIncomingMessage", Logger::L_Warning,
                        "method terminated with error code %d when handling message of type",
                        rc, pDisServiceMsg->getType());
    }
}

int ChunkRetrievalController::replyToQuery (ChunkRetrievalMsgQuery *pCRMQ, const char *pszIncomingInterface)
{
    if (pCRMQ == NULL) {
        return -1;
    }
    const char *pszTargetNodeId = pCRMQ->getSenderNodeId();
    if (pszTargetNodeId == NULL) {
        return -2;
    }
    const char *pszQueryId = pCRMQ->getQueryId();
    if (pszQueryId == NULL) {
        return -3;
    }

    // Check if the data is in cache. In the case, reply with a Query Hit Message.
    PtrLList<MessageHeader> *pChunks = _pDataCacheInterface->getCompleteChunkMessageInfos (pszQueryId);
    if (pChunks != NULL) {
        char **ppszOutgoingInterfaces = createOutgoingInterfacesList (pszIncomingInterface);
        ChunkRetrievalMsgQueryHits crmqh (_pDisService->getNodeId(), pszTargetNodeId, pChunks, pszQueryId, false);
        int rc = broadcastCtrlMessage (&crmqh, (const char**) NULL, "Sending Hit Query Msg");
        if (ppszOutgoingInterfaces != NULL) {
            for (int i = 0; ppszOutgoingInterfaces[i] != NULL; i++) {
                free (ppszOutgoingInterfaces[i]);
            }
            free (ppszOutgoingInterfaces);
            ppszOutgoingInterfaces = NULL;
        }

        // The Query Hit Message must be received only from the sender of the Query Message
        _pDataCacheInterface->release (pChunks);
        return rc;
    }

    return 0;
}

int ChunkRetrievalController::replyToQueryHits (ChunkRetrievalMsgQueryHits *pCRMQH)
{
    const char *pszSenderNodeId = pCRMQH->getSenderNodeId();
    if (pszSenderNodeId == NULL) {
        return -1;
    }
    if (pCRMQH->getTargetNodeId() == NULL) {
        return -2;
    }
    // it been received a hit for pszQueryId
    const char *pszQueryId = pCRMQH->getQueryId();
    if (pszQueryId == NULL) {
        return -4;
    }
    PtrLList<MessageHeader> *pCMIs = pCRMQH->getMessageHeaders();
    if (pCMIs == NULL) {
        return -5;
    }

    if (_pDiscoveryCtrl->hasReceivedHitForQuery (pszQueryId)) {
        checkAndLogMsg ("ChunkRetrievalController::replyToQueryHits",
                        Logger::L_Info, "non requesting %s to peer %s because a hit was already received\n",
                        pszQueryId, pszSenderNodeId);
        // Ignore the new hit (TODO: may store the message for later, in case more chunks are needed...)
        return 0;
    }
    for (MessageHeader *pMH = pCMIs->getFirst(); pMH != NULL; pMH = pCMIs->getNext()) {
        if (!_pDataCacheInterface->hasCompleteMessage (pMH) &&
            !_pDataCacheInterface->containsMessage (pMH)) {
            checkAndLogMsg ("ChunkRetrievalController::replyToQueryHits",
                            Logger::L_Info, "requesting %s to peer %s\n",
                            pMH->getMsgId(), pszSenderNodeId);
            _pDisService->historyRequest (0,               // client ID
                                          pMH->getMsgId(), // pszMessageID
                                          0);              // time out
            break;  // Force requesting only one chunk
        }
    }

    return 0;
}

ChunkRetrievalController::ContainerHitsList::ContainerHitsList (PtrLList<ChunkRetrievalController::Hit> *pListHits)
{
    _pListHits = pListHits;
}

ChunkRetrievalController::ContainerHitsList::~ContainerHitsList (void)
{
    // Deallocated in the destructor ~ChunkRetrievalController (void)
}

ChunkRetrievalController::Hit::Hit (const char *pszSenderNodeId, MessageHeader *pMH)
{
    _pszSenderNodeId = pszSenderNodeId;
    _pMH = pMH;
}

ChunkRetrievalController::Hit::~Hit (void)
{
    // Deallocated in the destructor ~ChunkRetrievalController (void)
}        

//==============================================================================
// ChunkDiscoveryController
//==============================================================================

ChunkDiscoveryController::ChunkDiscoveryController (DisseminationService *pDisService,
                                                    DataCacheInterface *pDataCacheInterface)
    : MessagingService (pDisService),
      _m (3),
      _discovering (true,    // bCaseSensitiveKeys
                    true,    // bCloneKeys
                    true)    // bDeleteKeys
{
    _pDisService = pDisService;
    _pDataCacheInterface = pDataCacheInterface;
}

ChunkDiscoveryController::~ChunkDiscoveryController()
{
}

bool ChunkDiscoveryController::hasReceivedHitForQuery (const char *pszQueryId)
{
    _m.lock (5);
    bool bRcvd = !_discovering.containsKey (pszQueryId);
    _m.unlock (5);

    return bRcvd;
}

void ChunkDiscoveryController::newIncomingMessage (const void *, uint16, DisServiceMsg *pDisServiceMsg,
                                                   uint32, const char *)
{
    if (pDisServiceMsg == NULL) {
        checkAndLogMsg ("ChunkDiscoveryController::newIncomingMessage",
                        Logger::L_SevereError, "pDisServiceMsg is null\n");
        return;
    }

    if (pDisServiceMsg->getType() != DisServiceCtrlMsg::CRMT_QueryHits) {
        return;
    }

    ChunkRetrievalMsgQueryHits *pCRMQH = (ChunkRetrievalMsgQueryHits*) pDisServiceMsg;
    const char *pszQueryId = (pCRMQH->getQueryId());
    if (pszQueryId == NULL) {
        checkAndLogMsg ("ChunkDiscoveryController::newIncomingMessage", Logger::L_Warning,
                        "received hit with empty pszQueryId\n");
        return;
    }

    // Check whether the hit message contains any matching message ID
    // The query ID (pszQueryId) may be removed from the hashset (_discovering)
    // upon reception of at least one non-empty hit.
    PtrLList<MessageHeader> *pCMIs = pCRMQH->getMessageHeaders();
    if (pCMIs == NULL) {
        checkAndLogMsg ("ChunkDiscoveryController::newIncomingMessage", Logger::L_Info,
                        "received empty hit for <%s>\n", pszQueryId);
        return;
    }

    if (pCMIs->getFirst() == NULL) {
        checkAndLogMsg ("ChunkDiscoveryController::newIncomingMessage", Logger::L_Info,
                        "received empty hit 2 for <%s>\n", pszQueryId);
        return;
    }

    String chunkIds ("[");
    if (pCMIs != NULL) {
        for (MessageHeader *pCurr = pCMIs->getFirst(); pCurr != NULL; pCurr = pCMIs->getNext()) {
            chunkIds += ((uint32) pCurr->getChunkId());
            chunkIds += " ";
        }
    }
    chunkIds += "]";

    bool bCanRemove = false;
    for (MessageHeader *pMH = pCMIs->getFirst(); pMH != NULL; pMH = pCMIs->getNext()) {
        if (!_pDataCacheInterface->hasCompleteMessage (pMH) &&
            !_pDataCacheInterface->containsMessage (pMH)) {
            bCanRemove = true;
            break;
        }
    }

    if (bCanRemove) {
        _m.lock (6);
        int64 *pTimeout = _discovering.remove (pszQueryId);
        if (pTimeout != NULL) {
            delete pTimeout;
            checkAndLogMsg ("ChunkDiscoveryController::newIncomingMessage", Logger::L_Info,
                            "received hit for <%s>; the discovered chunks are %s. Discovering "
                            "ID removed from the hashset.\n", pszQueryId, chunkIds.c_str());
        }
        _m.unlock (6);
    }
}

void ChunkDiscoveryController::retrieve (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId, int64 i64Timeout)
{
    if (pszGroupName == NULL || pszSenderNodeId == NULL) {
        return;
    }
    char *pszMsgId = convertFieldToKey (pszGroupName, pszSenderNodeId, ui32MsgSeqId);

    retrieveInternal (pszMsgId, i64Timeout, true);
    free (pszMsgId);    // _discovering makes a copy of the id
}

void ChunkDiscoveryController::retrieve (const char *pszId, int64 i64Timeout)
{
    retrieveInternal (pszId, i64Timeout, true);
}

void ChunkDiscoveryController::retrieveInternal (const char *pszId, int64 i64Timeout, bool bLock)
{
    if (pszId == NULL) {
        return;
    }
    const int64 i64RelativeTimeout = i64Timeout;
    if (i64Timeout < 0) {
        i64Timeout = 0;
    }
    else if (i64Timeout > 0) {
        i64Timeout += getTimeInMilliseconds();
    }

    DArray2<String> tokenizedKey;
    if (convertKeyToField (pszId, tokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM) < 0) {
        return;
    }
    unsigned int uiTotNChunks = 0;
    int iNChunks = _pDataCacheInterface->countChunks ((const char *) tokenizedKey[MSG_ID_GROUP],
                                                      (const char *) tokenizedKey[MSG_ID_SENDER],
                                                      (uint32) atoi (tokenizedKey[MSG_ID_SEQ_NUM]),
                                                      uiTotNChunks);
    if ((iNChunks < 0) ||             // Error
        ((((unsigned int) iNChunks) == uiTotNChunks) && (uiTotNChunks > 0))) {
        // All the chunks have already been received
        return;
    }

    PtrLList<MessageHeader> *pChunks = _pDataCacheInterface->getCompleteChunkMessageInfos ((const char *) tokenizedKey[MSG_ID_GROUP],
                                                                                           (const char *) tokenizedKey[MSG_ID_SENDER],
                                                                                           (uint32) atoi (tokenizedKey[MSG_ID_SEQ_NUM]));

    ChunkRetrievalMsgQuery crmq (_pDisService->getNodeId(), pszId, pChunks);
    int rc = broadcastCtrlMessage (&crmq, "Sending Retrieval Query Msg");

    String chunkIds ("[");
    if (pChunks != NULL) {
        MessageHeader *pNext = pChunks->getFirst();
        for (MessageHeader *pCurr; (pCurr = pNext) != NULL; ) {
            pNext = pChunks->getNext();
            chunkIds += ((uint32) pCurr->getChunkId());
            chunkIds += " ";
            delete pChunks->remove (pCurr);
        }
        delete pChunks;
        pChunks = NULL;
    }
    chunkIds += "]";

    if (rc < 0) {
        checkAndLogMsg ("ChunkDiscoveryController::retrieve", Logger::L_Warning,
                        "broadcastCtrlMessage failed with error code %d.\n", rc);
    }
    else {
        checkAndLogMsg ("ChunkDiscoveryController::retrieve", Logger::L_Info,
                        "sent message query for more chunks for message %s. The locally "
                        "cached chunks are %s.\n", pszId, chunkIds.c_str());
    }

    if (bLock) {
        _m.lock (7);
    }
    if (!_discovering.containsKey (pszId)) {
        checkAndLogMsg ("ChunkDiscoveryController::retrieve", Logger::L_Info,
                        "query ID <%s> added to discovery hashset with timeout %lld\n. The locally "
                        "cached chunks are %s.\n", pszId, i64RelativeTimeout, chunkIds.c_str());
        int64 *pTimeout = new int64;
        if (pTimeout != NULL) {
            *pTimeout = i64Timeout;
            _discovering.put (pszId, pTimeout);
        }
    }
    if (bLock) {
        _m.unlock (7);
    }
}

void ChunkDiscoveryController::sendDiscoveryMessage()
{
    _m.lock (8);
    if (_discovering.getCount() == 0U) {
        _m.unlock (8);
        return;
    }

    const int64 i64CurrentTime = getTimeInMilliseconds();
    StringHashtable<int64>::Iterator iter = _discovering.getAllElements();
    do {
        int64 *pI64Timeout = iter.getValue();
        if ((pI64Timeout == NULL) || (*pI64Timeout == 0) || (i64CurrentTime < (*pI64Timeout))) {
            checkAndLogMsg ("ChunkDiscoveryController::sendDiscoveryMessage", Logger::L_Info,
                            "sending discovery for %s (%u messages to be discovered).\n", iter.getKey(), _discovering.getCount());
            retrieveInternal (iter.getKey(), (pI64Timeout == NULL ? 0 : *pI64Timeout), false);
        }
        iter.nextElement();
    } while (!iter.end());

    _m.unlock (8);
}

