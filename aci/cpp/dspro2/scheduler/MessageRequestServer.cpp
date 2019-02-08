/*
 * MessageRequestServer.cpp
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
 * Created on November 1, 2012, 9:50 PM
 */

#include "MessageRequestServer.h"

#include "DataStore.h"
#include "DSSFLib.h"

#include "PtrLList.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

MessageRequestServer::MessageRequestServer (DataStore *pDataStore)
    : _pDataStore (pDataStore)
{
}

MessageRequestServer::~MessageRequestServer()
{
}

Message * MessageRequestServer::getRequestReply (const char *pszRequestedMsgId, uint8 *pChunkIdFilters,
                                                 uint8 ui8ChunkIdFiltersLen)
{
    if (pszRequestedMsgId == nullptr) {
        return nullptr;
    }
    if (isAllChunksMessageID (pszRequestedMsgId) && isOnDemandDataID (pszRequestedMsgId)) {
        return getAnyMatchingChunk (pszRequestedMsgId, pChunkIdFilters, ui8ChunkIdFiltersLen);
    }
    return _pDataStore->getCompleteMessage (pszRequestedMsgId);
}

Message * MessageRequestServer::getAnyMatchingChunk (const char *pszRequestedMsgId, uint8 *pChunkIdFilters,
                                                     uint8 ui8ChunkIdFiltersLen)
{
    // A chunked message was requested - send one of the chunks that have
    // not yet been received by the requestor
    PtrLList<Message> *pChunks = _pDataStore->getCompleteChunks (pszRequestedMsgId);
    if (pChunks == nullptr) {
        return nullptr;
    }

    if (pChunks->getFirst() == nullptr) {
        _pDataStore->releaseChunks (pChunks);
        return nullptr;
    }

    if (pChunkIdFilters == nullptr || ui8ChunkIdFiltersLen == 0) {
        Message *pMsg = pChunks->getFirst()->clone();
        _pDataStore->releaseChunks (pChunks);
        return pMsg;
    }

    Message *pMsg = nullptr;
    for (Message *pTmp = pChunks->getFirst(); pTmp != nullptr; pTmp = pChunks->getNext()) {
        bool bFound = false;
        for (uint8 i = 0; i < ui8ChunkIdFiltersLen && !bFound; i++) {
            if (pChunkIdFilters[i] == pTmp->getMessageHeader()->getChunkId()) {
                bFound = true;
            }
        }
        if (!bFound) {
            pMsg = pTmp->clone();
            break;
        }
    }

    _pDataStore->releaseChunks (pChunks);
    return pMsg;
}
