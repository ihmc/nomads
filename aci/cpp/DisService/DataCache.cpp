/*
 * DataCache.cpp
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

#include "DataCache.h"

#include "ChunkingAdaptor.h"
#include "DisseminationService.h"
#include "DisServiceDataCacheQuery.h"
#include "DisServiceDefs.h"
#include "Message.h"

#if defined (USE_SQLITE)
    #include "SQLMessageHeaderStorage.h"
#else
    #include "Storage.h"
#endif

#include "DSSFLib.h"

#include "FileReader.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

DataCache::DataCache()
    : _dataCache (true, true, true, true)
{
    // Setting cache's size parameters to the default value 0
    // if the values of those parameters is 0 it means there are
    // no size constraints for the cache
    _pDB = new SQLMessageHeaderStorage();
    _pDB->init();
}

DataCache::~DataCache (void)
{
    _dataCache.removeAll();
}

MessageHeader * DataCache::getMessageInfo (const char *pszId)
{
    return DataCacheInterface::getMessageInfo (pszId);
}

int DataCache::release (const char *pszId, MessageHeader *pMI)
{
    delete pMI;
    return 0;
}

void DataCache::clear (void)
{
    _m.lock (33);
    _pDB->clear();
    _dataCache.removeAll();
    _m.unlock (33);
}

void DataCache::getData (const char *pszId, Result &result)
{
    _m.lock (20);
    getDataInternal (pszId, result);
    _m.unlock (20);
}

void DataCache::getDataInternal (const char *pszId, Result &result)
{
    if (pszId == NULL) {
        return;
    }

    result.pData = NULL;
    result.ui32Length = 0;
    result.ui8StorageType = NOSTORAGETYPE;

    if (isAllChunksMessageID (pszId)) {
        DArray2<String> aTokenizedKey ((unsigned int)3);
        if (convertKeyToField (pszId, aTokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM) < 0) {
            checkAndLogMsg ("DataCache::getDataInternal", Logger::L_SevereError,
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
            pChunks = NULL;
            return;
        }

        PtrLList<MessageHeader> *pTmp = _pDB->getAnnotationsOnMsgMessageInfos (pszId);
        PtrLList<Message> *pAnnotations = getMessages (pTmp);
        delete pTmp;

        if (pChunks->getFirst()->getMessageHeader()->isChunk()) {
            // Return the reassembled large object
            uint32 ui32LargeObjLen = 0;
            const uint8 ui8Chunks = pChunks->getCount();
            const MessageHeader *pMH = pChunks->getFirst()->getMessageHeader();
            const uint8 ui8TotChunks = pMH->getTotalNumberOfChunks();
            const String objectId (pMH->getObjectId());
            const String instanceId (pMH->getInstanceId());

            Reader *pReader = ChunkingAdaptor::reassemble (_pChunkingMgr, pChunks, pAnnotations, ui32LargeObjLen);
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
            result.ui8NChunks = ui8Chunks;
            result.ui8TotalNChunks = ui8TotChunks;
            result.objectId = objectId;
            result.instanceId = instanceId;
            delete pReader;
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
    EntryHeader *pEntry = _dataCache.get (pszId);
    if (pEntry == NULL) {
        return;
    }

    switch (pEntry->ui8Type) {
        case DC_Entry:
            result.ui8StorageType = MEMORY;
            result.pData = malloc (pEntry->ui32Length);
            if (result.pData != NULL) {
                memcpy (result.pData, ((Entry*)pEntry)->pData, pEntry->ui32Length);
            }
            break;

        case DC_FileDataEntry:
            result.ui8StorageType = FILE;
            result.pData = ((FileEntry*)pEntry)->pszFilePath;
            break;
    }

    result.ui32Length = pEntry->ui32Length;
}

int DataCache::release (const char *pszId, Result &result)
{
    if (result.pData != NULL) {
        free (result.pData);
        result.pData = NULL;
    }
    result.ui8StorageType = NOSTORAGETYPE;
    result.ui32Length = 0;
    return 0;
}

const void * DataCache::getData (const char *pszId)
{
    uint32 ui32;
    return getData (pszId, ui32);
}

const void * DataCache::getData (const char *pszId, uint32 &ui32Len)
{
    ui32Len = 0;
    if (pszId == NULL) {
        return NULL;
    }

    _m.lock (22);
    Result result;
    getDataInternal (pszId, result);
    if (result.pData == NULL) {
        _m.unlock (22);
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
                                "error reading file %s\n", (char*) result.pData);
                free (pData);
                pData = NULL;
            }
            free ((void*) result.pData);
            break;
        }

        case NOSTORAGETYPE:
        default:
            break;
    }

    if (pData != NULL) {
        ui32Len = result.ui32Length;
    }

    _m.unlock (22);
    return pData;
}

int DataCache::release (const char *pszId, void *pData)
{
    if (pData != NULL) {
        free (pData);
    }
    return 0;
}

PtrLList<Message> * DataCache::getMatchingFragments (const char *pszGroupName, const char *pszSenderNodeId,
                                                     uint32 ui32MsgSeqId, uint8 ui8ChunkId, uint32 ui32StartOffset,
                                                     uint32 ui32EndOffset)
{
    _m.lock (24);
    PtrLList<Message> *pMessages = DataCacheInterface::getMatchingFragments (pszGroupName, pszSenderNodeId,
                                                                             ui32MsgSeqId, ui8ChunkId, ui32StartOffset,
                                                                             ui32EndOffset);
    _m.unlock (24);
    return pMessages;
}

PtrLList<Message> * DataCache::getMessages (const char *pszGroupName, const char *pszSenderNodeId,
                                            uint16 ui16Tag,
                                            uint32 ui32StartSeqNo, uint32 ui32EndSeqNo)
{
    _m.lock (25);
    PtrLList<MessageHeader> *pMIL = DataCacheInterface::getMessageInfos (pszGroupName, pszSenderNodeId,
                                                                         0,  // client type
                                                                         ui16Tag, ui32StartSeqNo,
                                                                         ui32EndSeqNo);
    PtrLList<Message> *pRet = getMessages (pMIL);
    // pMIL is emptied by getMessageInfos(), therefore I only need to deallocate
    // the point list
    delete pMIL;
    pMIL = NULL;
    _m.unlock (25);
    return pRet;
}

PtrLList<Message> * DataCache::getCompleteMessages (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32SeqId)
{
    _m.lock (26);
    PtrLList<Message> *pRet = getCompleteMessagesInternals (pszGroupName, pszSenderNodeId, ui32SeqId);
    _m.unlock (26);
    return pRet;

}

PtrLList<Message> * DataCache::getCompleteMessagesInternals (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32SeqId)
{
    if (_pDB == NULL) {
        return NULL;
    }
    PtrLList<MessageHeader> *pChunks = _pDB->getCompleteChunkMessageInfos (pszGroupName, pszSenderNodeId, ui32SeqId);
    PtrLList<Message> *pRet = getMessages (pChunks);
    delete pChunks;
    return pRet;
}

PtrLList<Message> * DataCache::getMessages (NOMADSUtil::PtrLList<MessageHeader> *pMHs)
{
    if (pMHs == NULL || pMHs->getFirst() == NULL) {
        return NULL;
    }
    MessageHeader *pTmpMH = pMHs->getFirst();
    if (pTmpMH == NULL) {
        return NULL;
    }

    PtrLList<Message> *pRet = new PtrLList<Message>();
    MessageHeader *pMH;
    while ((pMH = pTmpMH) != NULL) {
        pTmpMH = pMHs->getNext();
        pMHs->remove (pMH);
        const void *pData = getData (pMH->getMsgId());
        assert (pData != NULL);
        Message *pMsg = new Message (pMH, pData);
        if (pMsg == NULL) {
            checkAndLogMsg ("DataCache::getMessages", memoryExhausted);
            break;
        }
        pRet->prepend (pMsg);
    }

    if (pRet->getFirst() == NULL) {
        delete pRet;
        pRet = NULL;
    }

    return pRet;
}

int DataCache::release (PtrLList<Message> *pMessages)
{
    if (pMessages == NULL) {
        return -1;
    }

    Message *pMsg;
    Message *pMsgTmp = pMessages->getFirst();
    while ((pMsg = pMsgTmp) != NULL) {
        // Delete current element and get next
        pMsgTmp = pMessages->getNext();
        pMessages->remove (pMsg);
        release (pMsg);
    }
    delete pMessages;
    return 0;
}

int DataCache::release (Message *pMsg)
{
    delete pMsg->getMessageHeader();
    if (pMsg->getData() != NULL) {
        free ((void*)pMsg->getData());
    }
    delete pMsg;
    return 0;
}

DArray2<String> * DataCache::getSenderNodeIds (const char *pszGroupName)
{
    _m.lock (28);
    DArray2<String> *pIDs = DataCacheInterface::getSenderNodeIds (pszGroupName);
    _m.unlock (28);
    return pIDs;
}

int DataCache::release (PtrLList<MessageHeader> *pMessageInfos)
{
    MessageHeader *pMH;
    MessageHeader *pMITmp = pMessageInfos->getFirst();
    while ((pMH = pMITmp) != NULL) {
        pMITmp = pMessageInfos->getNext();
        pMessageInfos->remove (pMH);
        delete pMH;
    }
    return 0;
}

DArray2<String> * DataCache::getRecentPublishers (uint16 ui16HistoryLength, const char * pszGroupName, uint16 ui16Tag)
{
    _m.lock (29);
    DArray2<String> *pResult;
    #if defined (USE_SQLITE)
        DArray2<String> groupByFields (1);
        groupByFields[0] = SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID;
        pResult = _pDB->execSender (pszGroupName, 0, 0, 0, 0, 0, 0, 0, ui16HistoryLength,
                                    SQLMessageHeaderStorage::FIELD_ARRIVAL_TIMESTAMP, true, &groupByFields);
    #else
        const char * pszMethodName = "DataCache::getRecentPublishers";
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "method not implemented yet.\n");
        pResult = NULL;
    #endif
    _m.unlock (29);
    return pResult;
}

DArray2<String> * DataCache::getExpiredEntries()
{
    if (_pDB == NULL) {
        return NULL;
    }

    return _pDB->getExpiredEntries();
}

int DataCache::addDataNoNotifyInternal (MessageHeader *pMessageHeader, const void *pData,
                                        unsigned int uiListenerID)
{
    Entry *pEntry = new Entry();
    pEntry->ui8Type = DC_Entry;
    pEntry->ui32Length = pMessageHeader->getFragmentLength();
    pEntry->pData = malloc (pEntry->ui32Length);
    memcpy (pEntry->pData, pData, pEntry->ui32Length);

    int rc = addDataNoNotifyInternal (pMessageHeader, pEntry, uiListenerID);
    if (rc != 0) {
        delete pEntry;
        return rc;
    }

    // pMessageHeader and pData will be deleted when this method returns
    return rc;
}

int DataCache::addDataNoNotifyInternal (MessageHeader *pMessageHeader, Entry *pEntry,
                                        unsigned int uiListenerID)
{
    const char *pszMethodName = "DataCache::addDataNoNotifyInternal";

    if (_dataCache.containsKey (pMessageHeader->getMsgId())) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "Message <%s> already in the cache.\n",
                        pMessageHeader->getMsgId());
        return -1;
    }

    if (false == cleanCache (pEntry->ui32Length, pMessageHeader, pEntry->pData)) {
        return -2;
    }

    Message msg (pMessageHeader, pEntry->pData);
    if (0 == _pDB->insert (&msg)) {
        _dataCache.put (pMessageHeader->getMsgId(), pEntry);
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                        "Added message with id <%s> to the DataCache.\n",
                        pMessageHeader->getMsgId());
        _ui32CurrentCacheSize += pEntry->getLength();
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "insert into _pDB failed.\n");
        return -3;
    }

    // If it is a complete message all the fragments can be deleted from the cache
    if (pMessageHeader->isCompleteMessage()) {
        uint32 ui32FragmentCounter = 0;
        DArray2<String> keysToEliminate;
        int rc = _pDB->eliminateAllTheMessageFragments (pMessageHeader->getGroupName(), pMessageHeader->getPublisherNodeId(),
                                                        pMessageHeader->getMsgSeqId(), pMessageHeader->getChunkId(),
                                                        &keysToEliminate);
        if (rc == 0) {
            for (unsigned int i = 0; i < keysToEliminate.size(); i++) {
                if (keysToEliminate.used (i)) {
                    if (deleteDataFromCache (keysToEliminate[i]) == 0) {
                        ui32FragmentCounter++;
                    }
                }
                else {
                    checkAndLogMsg (pszMethodName, Logger::L_Warning,
                                    "Unused slot in pKeysToEliminate.\n");
                }
            }
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "eliminateAllTheMessageFragments returned an error: %d\n", rc);
        }
    }

    return 0;
}

int DataCache::deleteDataFromCache (const char *pszKey)
{
    String key (pszKey);
    Entry *pEntry = _dataCache.remove (key);
    if (pEntry == NULL) {
        checkAndLogMsg ("DataCache::deleteDataFromCache", Logger::L_Warning,
                        "Attempt to delete <%s> from cache: NOT FOUND!\n",
                        pszKey);
        return -1;
    }
    else {
        _ui32CurrentCacheSize = _ui32CurrentCacheSize - (pEntry->ui32Length);
        delete pEntry;
        return 0;
    }
}

int DataCache::deleteDataAndMessageInfo (const char *pszKey, bool bIsLatestMessagePushedByNode)
{
    _m.lock (31);
    _pDB->eliminate (pszKey);
    int ret = deleteDataFromCache (pszKey);

    _m.unlock (31);
    return ret;
}

//==============================================================================
//  Entries
//==============================================================================

DataCache::EntryHeader::EntryHeader (EntryType ui8EntryType)
{
    ui8Type = ui8EntryType;
}

DataCache::EntryHeader::~EntryHeader()
{
}

//------------------------------------------------------------------------------

DataCache::Entry::Entry()
    : EntryHeader (DC_Entry)
{
    pData = NULL;
    ui32Length = 0;
}

DataCache::Entry::~Entry()
{
    free (pData);
    pData = NULL;
    ui32Length = 0;
}

uint32 DataCache::Entry::getLength()
{
    return (sizeof(ui32Length) + sizeof(ui8Type) + ui32Length);
}

//------------------------------------------------------------------------------

DataCache::FileEntry::FileEntry()
    : EntryHeader (DC_FileDataEntry)
{
    pszFilePath = NULL;
}

DataCache::FileEntry::~FileEntry()
{
    free (pszFilePath);
    pszFilePath = NULL;
}

uint32 DataCache::FileEntry::getLength()
{
    return (sizeof(ui32Length) + sizeof(ui8Type) + (uint32) strlen (pszFilePath));
}

