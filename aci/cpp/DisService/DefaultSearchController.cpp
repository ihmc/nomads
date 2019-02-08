/*
 * DefaultSearchController.cpp
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on March 13, 2014, 1:42 PM
 */

#include "DefaultSearchController.h"

#include "DataCacheInterface.h"
#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "DSSFLib.h"
#include "MessageReassembler.h"
#include "Searches.h"

#include "DArray2.h"
#include "NLFLib.h"
#include "RangeDLList.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

DefaultSearchController::DefaultSearchController (DisseminationService *pDisService,
                                                  DataCacheInterface *pDataCache,
                                                  MessageReassembler *pMessageReassembler)
    : SearchController (pDisService),
      _nodeId (pDisService->getNodeId()),
      _pDataCache (pDataCache),
      _pMessageReassembler (pMessageReassembler)
{
}

DefaultSearchController::~DefaultSearchController()
{
}

void DefaultSearchController::searchArrived (const char *pszQueryId, const char *pszGroupName,
                                             const char *pszQuerier, const char *pszQueryType,
                                             const char *, const void *pszQuery, unsigned int uiQueryLen)
{
    if (pszQueryId == NULL || pszGroupName == NULL || pszQuerier == NULL ||
        pszQueryType == NULL || pszQuery == NULL || uiQueryLen == 0) {
        return;
    }

    if (pszQuerier == NULL) {
        pszQuerier = _nodeId;
    }
    Searches::getSearches()->receivedSearchInfo (pszQueryId, pszQueryType, pszQuerier);
}

void DefaultSearchController::searchReplyArrived (const char *pszQueryId,
                                                  const char **ppszMatchingMessageIds,
                                                  const char *pszMatchingNodeId)
{
    const char *pszMethodName = "DefaultSearchController::searchReplyArrived";
    if ((pszQueryId == NULL) || (ppszMatchingMessageIds == NULL) || (pszMatchingNodeId == NULL)) {
        return;
    }

    uint16 ui16ClientId = Searches::FORWARDED_SEARCH_CLIENT_ID;
    String queryType;
    String querier;
    if (0 != Searches::getSearches()->getSearchInfo (pszQueryId, queryType, querier, ui16ClientId)) {
        // The search was never forwarded by this node - do not process its
        // corresponding search reply
        return;
    }

    for (unsigned int i = 0; ppszMatchingMessageIds[i] != NULL; i++) {
        bool bRequestMsg = true;
        if (isAllChunksMessageID (ppszMatchingMessageIds[i])) {
            // Check whether _any_ of the chunks has already been received. If so,
            // the other chunks should not be requested. Deciding whether to retrieve
            // more chunks is up to the application.
            bRequestMsg = !_pDataCache->hasCompleteMessageOrAnyCompleteChunk (ppszMatchingMessageIds[i]);
        }
        else {
            bRequestMsg = !_pDataCache->hasCompleteMessage (ppszMatchingMessageIds[i]);
        }
        if (bRequestMsg) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "sending message request for "
                            "match %s to search %s\n", ppszMatchingMessageIds[i], pszQueryId);
            if (_nodeId != querier) {
                ui16ClientId = Searches::FORWARDED_SEARCH_CLIENT_ID;
            }
            sendMessageRequest (ppszMatchingMessageIds[i], pszQueryId, ui16ClientId);
        }
        else if (Searches::getSearches()->isSearchFromPeer (pszQueryId, _nodeId)) {
            uint16 ui16ClientId = 0;
            if (Searches::getSearches()->getSearchQueryId (pszQueryId, ui16ClientId) == 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "scheduling client %u to be "
                                "notified match %s for search %s\n", ui16ClientId, ppszMatchingMessageIds[i],
                                pszQueryId);
                notifyClients (ppszMatchingMessageIds[i], pszQueryId, ui16ClientId);
            }
        }
    }
}

void DefaultSearchController::volatileSearchReplyArrived (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen, const char *pszMatchingNodeId)
{
    // TODO: implement this
}

void DefaultSearchController::sendMessageRequest (const char *pszMsgId, const char *pszQueryId, uint16 ui16ClientId)
{
    DArray2<String> tokens;
    uint8 ui8ChunkSeqId = MessageHeader::UNDEFINED_CHUNK_ID;
    int rc;
    if (isAllChunksMessageID (pszMsgId)) {
        rc = convertKeyToField (pszMsgId, tokens, 3,
                                MSG_ID_GROUP, MSG_ID_SENDER,
                                MSG_ID_SEQ_NUM);
    }
    else {
        rc = convertKeyToField (pszMsgId, tokens, 4,
                                MSG_ID_GROUP, MSG_ID_SENDER,
                                MSG_ID_SEQ_NUM, MSG_ID_CHUNK_ID);
        ui8ChunkSeqId = (uint8) atoui32 (tokens[MSG_ID_CHUNK_ID]);
    }
    if (rc < 0) {
        return;
    }

    uint32 ui32MsgSeqId = atoui32 (tokens[MSG_ID_SEQ_NUM]);
    RequestInfo reqInfo (ui16ClientId);
    reqInfo._pszGroupName = tokens[MSG_ID_GROUP];
    reqInfo._pszSenderNodeId = tokens[MSG_ID_SENDER];
    reqInfo._ui64ExpirationTime = 0;
    if (reqInfo._ui64ExpirationTime > 0) {
        reqInfo._ui64ExpirationTime += getTimeInMilliseconds();
    }
    reqInfo._pszQueryId = pszQueryId;

    if (ui8ChunkSeqId == MessageHeader::UNDEFINED_CHUNK_ID) {
        UInt32RangeDLList ranges (true);
        ranges.addTSN (ui32MsgSeqId, ui32MsgSeqId);
        _pMessageReassembler->sendRequest (reqInfo, &ranges);
        _pMessageReassembler->addRequest (reqInfo, &ranges);
    }
    else {
        UInt8RangeDLList ranges (false);
        ranges.addTSN (ui8ChunkSeqId, ui8ChunkSeqId);
        _pMessageReassembler->sendRequest (reqInfo, ui32MsgSeqId, &ranges);
        _pMessageReassembler->addRequest (reqInfo, ui32MsgSeqId, &ranges);
    }
}

