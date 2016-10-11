/*
 * PersistentDataCache.cpp
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

#include "PersistentDataCache.h"

#include "DisseminationService.h"
#include "DisServiceDataCacheQuery.h"
#include "Message.h"
#include "MessageInfo.h"

#if defined (USE_SQLITE)
    #include "SQLMessageStorage.h"
#else
    #include "Storage.h"
#endif

#include "FileReader.h"
#include "DSSFLib.h"
#include "Logger.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_ACI;
using namespace NOMADSUtil;

PersistentDataCache::PersistentDataCache (bool bUseTransactionTimer)
{
    // Setting cache's size parameters to the default value 0
    // if the values of those parameters is 0 it means there are
    // no size constraints for the cache
    #if defined (USE_SQLITE)
        _pPersistentDB = new SQLMessageStorage ("db.sqlite", bUseTransactionTimer);
        _pPersistentDB->init();
    #else
        _pPersistentDB = new Storage();
    #endif
    _pDB = _pPersistentDB;
}

PersistentDataCache::PersistentDataCache (const char *pszDBName, bool bUseTransactionTimer)
{
    // Setting cache's size parameters to the default value 0
    // if the values of those parameters is 0 it means there are
    // no size constraints for the cache
    #if defined (USE_SQLITE)
        _pPersistentDB = new SQLMessageStorage (pszDBName, bUseTransactionTimer);
        _pPersistentDB->init();
    #else
        _pPersistentDB = new Storage();
    #endif
    _pDB = _pPersistentDB;
}

PersistentDataCache::~PersistentDataCache()
{
    // NOTE: _pPersistentDB and _pDB point to the same object!
    delete _pPersistentDB;
    _pPersistentDB = NULL;
    _pDB = NULL;
}

//------------------------------------------------------------------------------
// Methods to add data to the PersistentDataCache
//------------------------------------------------------------------------------

int PersistentDataCache::addDataNoNotifyInternal (MessageHeader *pMessageHeader,
                                                  const void *pData,
                                                  unsigned int uiListenerID)
{
    Message msg (pMessageHeader, pData);
    return addDataNoNotifyInternal (&msg);
}

PtrLList<Message> * PersistentDataCache::getCompleteMessages (const char *pszGroupName, const char *pszSenderNodeId,
                                                              uint32 ui32SeqId)
{
    return _pPersistentDB->getCompleteChunks (pszGroupName, pszSenderNodeId, ui32SeqId);
}

int PersistentDataCache::addDataNoNotifyInternal (Message *pMessage)
{
    const char *pszMethodName = "PersistentDataCache::addDataNoNotifyInternal";

    MessageHeader *pMH = pMessage->getMessageHeader();

    uint32 ui32FragmentCounter = 0;
    if (pMH->isCompleteMessage()) {
        _pPersistentDB->eliminateAllTheMessageFragments (pMH->getGroupName(), pMH->getPublisherNodeId(),
                                                         pMH->getMsgSeqId(), pMH->getChunkId(), NULL);
    }

    if (false == cleanCache (pMH->getTotalMessageLength(),
                             pMH, (void *)pMessage->getData())) {
        return -1;
    }

    if (0 == _pPersistentDB->insert (pMessage)) {
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                        "Added message with id <%s> to the DataCache. %s",
        	        pMH->getMsgId(),
                        (ui32FragmentCounter > 0 ? "Fragments for this complete message deleted.\n" : "\n"));
        _ui32CurrentCacheSize += pMH->getTotalMessageLength();
        return 0;
    }
    return -2;

}

//------------------------------------------------------------------------------
// Methods to query the PersistentDataCache
//------------------------------------------------------------------------------

int PersistentDataCache::release (const char *pszId, MessageHeader *pMI)
{
    delete pMI;
    pMI = NULL;
    return 0;
}

const void * PersistentDataCache::getData (const char *pszId)
{
    uint32 ui32 = 0;
    return getData (pszId, ui32);
}

const void * PersistentDataCache::getData (const char *pszId, uint32 &ui32Len)
{
    ui32Len = 0;
    if (pszId == NULL) {
        return NULL;
    }

    _m.lock (79);
    Result result;
    getDataInternal (pszId, result);
    if (result.pData == NULL) {
        _m.unlock (79);
        return NULL;
    }

    void *pData = NULL;
    switch (result.ui8StorageType) {
        case MEMORY:
            pData = result.pData;
            break;

        case FILE: {
            FileReader reader (fopen ((char *)result.pData, "r"), true);
            pData = malloc (result.ui32Length);
            if (pData == NULL) {
                checkAndLogMsg ("DataCache::getData", memoryExhausted);
            }
            if (reader.readBytes (pData, result.ui32Length) < 0) {
                checkAndLogMsg ("DataCache::getData", Logger::L_MildError,
                                "error reading file %s\n", (char *)result.pData);
                free (pData);
                pData = NULL;
            }
            break;
        }

        case NOSTORAGETYPE:
        default:
            break;
    }

    if (pData != NULL) {
        ui32Len = result.ui32Length;
    }

    _m.unlock (80);
    return pData;
}

int PersistentDataCache::release (const char *pszId, void *pData)
{
    if (pData != NULL) {
        free (pData);
        pData = NULL;
    }
    return 0;
}

void PersistentDataCache::getData (const char *pszId, Result &result)
{
    _m.lock (82);
    getDataInternal (pszId, result);
    _m.unlock (82);
}

void PersistentDataCache::getDataInternal (const char *pszId, Result &result)
{

    if (isAllChunksMessageID (pszId)) {
        DArray2<String> aTokenizedKey ((unsigned int)3);
        if (convertKeyToField (pszId, aTokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM) < 0) {
            checkAndLogMsg ("PersistentDataCache::getDataInternal", Logger::L_SevereError,
                            "convertKeyToField returned an error\n");
            return;
        }
        PtrLList<Message> *pChunks = getCompleteMessages (aTokenizedKey[MSG_ID_GROUP],
                                                          aTokenizedKey[MSG_ID_SENDER],
                                                          atoui32 (aTokenizedKey[MSG_ID_SEQ_NUM]));
        if (pChunks == NULL) {
            return;
        }
        if (pChunks->getFirst() == NULL) {
            release (pChunks);
        }

        PtrLList<Message> *pAnnotations = _pPersistentDB->getCompleteAnnotations (pszId);
        if (pChunks->getFirst()->getMessageHeader()->isChunk()) {
            // Return the reassembled large object
            uint32 ui32LargeObjLen = 0;
            Reader *pReader = ChunkingAdaptor::reassemble (pChunks, pAnnotations, ui32LargeObjLen);
            release (pChunks);
            if (pReader == NULL) {
                checkAndLogMsg ("DataCache::getDataInternal", Logger::L_SevereError,
                                "could not reassemble large object\n");
                return;
            }

            // TODO: for now always return the buffer in memory.
            //       it should be returned in a file if very large...
            result.pData = malloc (ui32LargeObjLen);
            if (result.pData == NULL) {
                checkAndLogMsg ("DataCache::getDataInternal", memoryExhausted);
                return;
            }
            pReader->read (result.pData, ui32LargeObjLen);
            result.ui8StorageType = MEMORY;
            result.ui32Length = ui32LargeObjLen;
        }
        else {
            assert (pChunks->getCount() == 1);
            MessageInfo *pMI = pChunks->getFirst()->getMessageInfo();
            result.ui8StorageType = MEMORY;
            result.pData = (void*)pChunks->getFirst()->getData();
            result.ui32Length = pMI->getFragmentLength();
        }
        
        return;
    }

    // Requesting for a specific message/fragment/chunk
    Message *pMsg = _pPersistentDB->getMessage (pszId);
    if (pMsg == NULL) {
        result.pData = NULL;
        result.ui32Length = 0;
        result.ui8StorageType = NOSTORAGETYPE;

        return;
    }

    MessageHeader *pMH = pMsg->getMessageHeader();
    if (pMH == NULL) {
        result.pData = NULL;
        result.ui32Length = 0;
        result.ui8StorageType = NOSTORAGETYPE;
        if (pMsg->getData() != NULL) {
            free ((void *) pMsg->getData());
        }
    }
    result.pData = (void *) pMsg->getData();
    if (result.pData == NULL) {
        result.pData = NULL;
        result.ui32Length = 0;
        result.ui8StorageType = NOSTORAGETYPE;
    }

    result.ui8StorageType = MEMORY;
    result.ui32Length = pMH == NULL ? 0 : pMH->getFragmentLength();

    delete pMsg->getMessageHeader();
    delete pMsg;
}

int PersistentDataCache::release (const char *pszId, Result &result)
{
    result.ui8StorageType = NOSTORAGETYPE;
    result.ui32Length = 0;
    if (result.pData != NULL) {
        free (result.pData);
    }

    return 0;
}

PtrLList<Message> * PersistentDataCache::getMessages (const char *pszGroupName, const char *pszSenderNodeId,
                                                      uint16 ui16Tag, uint32 ui32StartSeqNo, uint32 ui32EndSeqNo)
{
    DisServiceDataCacheQuery *pDSDCQuery = new DisServiceDataCacheQuery();
    pDSDCQuery->setPersistent();
    pDSDCQuery->selectAll();
    pDSDCQuery->addConstraintOnGroupName (pszGroupName);
    pDSDCQuery->addConstraintOnSenderNodeId (pszSenderNodeId);
    pDSDCQuery->addConstraintOnTag (ui16Tag);
    if (ui32StartSeqNo == ui32EndSeqNo) {
        pDSDCQuery->addConstraintOnMsgSeqId (ui32StartSeqNo);
    }
    else {
        pDSDCQuery->addConstraintOnMsgSeqId (ui32StartSeqNo, ui32EndSeqNo, 
                                             true);   // open interval
    }

    PtrLList<Message> *pRet = _pPersistentDB->getMessages (pDSDCQuery);

    delete pDSDCQuery;
    return pRet;
}

int PersistentDataCache::release (PtrLList<Message> *pMessages)
{
    Message *pMsg = pMessages->getFirst();
    Message *pMsgTmp; MessageHeader *pMH;
    while (pMsg) {
        // release the locked element
        pMH = pMsg->getMessageHeader();
        void *pData = (void *) pMsg->getData();

        // Delete current element and get next
        pMsgTmp = pMessages->getNext();
        delete pMH;
        pMH = NULL;
        free (pData);
        pData = NULL;
        delete pMsg;
        pMsg = pMsgTmp;
    }
    delete pMessages;
    return 0;
}

int PersistentDataCache::release (Message *pMsg)
{
    if (pMsg == NULL) {
        return -1;
    }

    delete pMsg->getMessageHeader();
    free ((void *) pMsg->getData());
    delete pMsg;
    return 0;
}

// Returns MESSAGEINFO List

int PersistentDataCache::release (PtrLList<MessageHeader> *pMessageInfos)
{
    MessageHeader *pMI = pMessageInfos->getFirst();
    MessageHeader *pMITmp;
    while (pMI) {
        // Delete current element and get next
        pMITmp = pMessageInfos->getNext();
        delete pMI;
        pMI = pMITmp;
    }
    return 0;
}

DArray2<String> * PersistentDataCache::getRecentPublishers (uint16 ui16HistoryLength, const char *pszGroupName, uint16 ui16Tag)
{
    DArray2<String> *pResult;
    DArray2<String> groupByFields (1);
    groupByFields[0] = SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID;
    _m.lock (85);
    pResult = _pPersistentDB->execSender (pszGroupName, 0, 0, 0, 0, 0, 0, 0,
                                          ui16HistoryLength,
                                          SQLMessageHeaderStorage::FIELD_ARRIVAL_TIMESTAMP,
                                          true, &groupByFields);
    _m.unlock (85);
    return pResult;
}

NOMADSUtil::DArray2<NOMADSUtil::String> * PersistentDataCache::getExpiredEntries()
{
    if (_pPersistentDB == NULL) {
        return NULL;
    }

    return _pPersistentDB->getExpiredEntries();
}

//==============================================================================
//  PRIVATE METHODS
//==============================================================================

int PersistentDataCache::deleteDataAndMessageInfo (const char *pszKey, bool bIsLatestMessagePushedByNode)
{
    
    if (bIsLatestMessagePushedByNode) {
        checkAndLogMsg ("PersistentDataCache::deleteDataAndMessageInfo", Logger::L_Info,
                        "the message %s can't be eliminated because it is the latest message sent.\n", pszKey);
        return -1;
    }
    _m.lock (88);
    int ret = _pPersistentDB->eliminate (pszKey);

    _m.unlock (88);
    return ret;
}
