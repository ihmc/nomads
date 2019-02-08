/*
 * ChunkQueryController.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 */

#include "ChunkQueryController.h"

#include "DSProImpl.h"
#include "ChunkQuery.h"
#include "DataStore.h"
#include "MessageIdGenerator.h"

#include "DSSFLib.h"

#include "MimeUtils.h"

#include "BufferReader.h"
#include "BufferWriter.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

namespace IHMC_ACI
{

}

ChunkQueryController::ChunkQueryController (DSProImpl *pDSPro, DataStore *pDataStore, InformationStore *pInfoStore)
    : QueryController ("ChunkQueryController", ChunkQuery::QUERY_TYPE, pDSPro, pDataStore, pInfoStore)
{

}

ChunkQueryController::~ChunkQueryController(void)
{
}

// SearchListener methods
void ChunkQueryController::searchArrived (const char *pszQueryId, const char *pszGroupName, const char *pszQuerier,
                                          const char *pszQueryType, const char *pszQueryQualifiers, const void *pQuery,
                                          unsigned int uiQueryLen)
{
    QueryController::searchArrived (pszQueryId, pszGroupName, pszQuerier, pszQueryType, pszQueryQualifiers, pQuery, uiQueryLen);

    // A search arrived from the application
    const char *pszMethodName = "ChunkQueryController::searchArrived";

    if (pQuery == nullptr || pszQueryType == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "some of the needed parameters are null\n");
        return;
    }

    if (!supportsQueryType (pszQueryType)) {
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "query type %s not supported\n", pszQueryType);
        return;
    }

    // Parse query
    ChunkQuery query;
    BufferReader br (pQuery, uiQueryLen, false);
    int rc = query.read (&br);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not read chunk query. Return code %d\n", rc);
    }

    // Check whether the query can be served
    char *ppszMatchingMsgIds[2] = { getMatchingId (query, getDSPro(), getDataStore()), nullptr };
    if (ppszMatchingMsgIds[0] != nullptr) {
        rc = notifySearchReply (pszQueryId, pszQuerier, const_cast<const char **>(ppszMatchingMsgIds), getDSPro()->getNodeId());
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "notifySearchReply() returned and error: %d\n", rc);
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "replying %s with custom chunk %s in response to query %s.\n",
                            pszQuerier, ppszMatchingMsgIds[0], pszQueryId);
        }
        free (ppszMatchingMsgIds[0]);
    }
}

void ChunkQueryController::searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId)
{
    /*const char *pszMethodName = "ChunkQueryController::searchReplyArrived";
    StringHashset *pMatchingPeers = _receivedHits.get (pszQueryId);
    if (pMatchingPeers == nullptr) {
        pMatchingPeers = new StringHashset();
        _receivedHits.put (pszQueryId, pMatchingPeers);
    }
    if (pMatchingPeers->containsKey (pszMatchingNodeId)) {
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "Dropping duplicate search "
                        "reply arrived for query %s from peer %s.\n", pszQueryId, pszMatchingNodeId);
    }
    else {*/
        QueryController::searchReplyArrived (pszQueryId, ppszMatchingMessageIds, pszMatchingNodeId);
    //    pMatchingPeers->put (pszMatchingNodeId);
    //}
}

char * ChunkQueryController::getMatchingId (ChunkQuery &query, DSProImpl *pDSPro, DataStore *pDataStore)
{
    const char *pszMethodName = "ChunkQueryController::getMatchingId";
    uint8 ui8NumberOfChunks = 0;
    uint8 ui8TotalNumberOfChunks = 0;
    if ((0 == pDataStore->getNumberOfReceivedChunks (query._objectMsgId, ui8NumberOfChunks, ui8TotalNumberOfChunks))
        && (ui8NumberOfChunks == ui8TotalNumberOfChunks)) {
        bool bHasMoreChunks = true;
        uint32 ui32DataLen = 0U;
        void *pData = nullptr;
        int rc = pDSPro->getData (query._objectMsgId, nullptr, &pData, ui32DataLen, bHasMoreChunks);
        if ((rc == 0) && (pData != nullptr) && (ui32DataLen > 0U) && (!bHasMoreChunks)) {
            Chunker::Interval **ppIntervals = query._chunkDescr.getIntervals();
            if (ppIntervals != nullptr) {
                Chunker::Fragment *pFragment = Chunker::extractFromBuffer (pData, ui32DataLen, query._chunkDescr._inputType,
                                                                           query._chunkDescr._outputType,
                                                                           query._ui8CompressionQuality, ppIntervals);
                CustomChunkDescription::deleteIntervals (ppIntervals);
                if (pFragment == nullptr) {
                    free (pData);
                    return nullptr;
                }
                if (pFragment->ui64FragLen > 0xFFFFFFFF) {
                    free (pData);
                    delete pFragment->pReader;
                    delete pFragment;
                    return nullptr;
                }
                const uint32 ui32FragLen = static_cast<uint32>(pFragment->ui64FragLen);
                void *pFragmentData = malloc (ui32FragLen);
                if (pFragment->pReader->read (pFragmentData, ui32FragLen) < 0) {
                    free (pData);
                    delete pFragment->pReader;
                    delete pFragment;
                    if (pFragmentData != nullptr) free (pFragmentData);
                    return nullptr;
                }
                delete pFragment->pReader;
                delete pFragment;
                String sSubGrpName (MessageIdGenerator::extractSubgroupFromMsgId (query._objectMsgId));
                if (isOnDemandGroupName (sSubGrpName)) {
                    // TODO: this is a hack! The "on demand" suffix is actually used to identify
                    // "chunked" messages in DSPro, while it should be only be used to indicate
                    // that the message was "on-demand". Chunks are always "on-demand", but it is
                    // not true that on-demand data is always chunked. Leaving the [od] suffix
                    // leads to requesting the object as a chunk, while in this case it is never
                    // chunked.
                    sSubGrpName = sSubGrpName.substring (0, sSubGrpName.lastIndexOf ('.'));
                    sSubGrpName += ".[cu]";
                }
                char *pszId = nullptr;
                const char *pszObjectId = nullptr;
                const char *pszInstanceId = nullptr;
                const String mimeType (MimeUtils::toMimeType (query._chunkDescr._outputType));
                const int64 i64ExpirationTime = 0;
                BufferWriter bw (1024, 1024);
                query._chunkDescr.write (&bw);
                int rc = pDSPro->addData (sSubGrpName, pszObjectId, pszInstanceId, query._objectMsgId,
                                          bw.getBuffer(), bw.getBufferLength(), pFragmentData, ui32FragLen,
                                          mimeType, i64ExpirationTime, &pszId);
                if (rc < 0) {
                    checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not store custum "
                                    "chunk on %s. Return code: %s.\n", query._objectMsgId.c_str(), rc);
                }
                else {
                    checkAndLogMsg (pszMethodName, Logger::L_Info, "stored custum chunk "
                                    "%s on %s.\n", pszId, query._objectMsgId.c_str(), rc);
                }
                if (pFragmentData != nullptr) {
                    free (pFragmentData);
                }
                if (pData != nullptr) {
                    free (pData);
                }
                return pszId;
            }
        }
        if (pData != nullptr) {
            free (pData);
        }
    }
    return nullptr;
}

