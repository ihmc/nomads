/*
 * DataStore.cpp
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
 */

#include "DataStore.h"

#include "DataCacheInterface.h"
#include "DSSFLib.h"
#include "MessageHeaders.h"
#include "MessageInfo.h"
#include "PersistentDataCache.h"

#include "ConfigManager.h"
#include "DArray2.h"
#include "Logger.h"
#include "NLFLib.h"
#include "StrClass.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const char * DataStore::PERSISTENCE_MODE = "dspro.persistence.mode";
const char * DataStore::PERSISTENCE_FILE = "dspro.persistence.filename";
const char * DataStore::PERSISTENCE_AUTOCOMMIT = "dspro.persistence.autocommit.enabled";

DataStore * DataStore::_pDataStore = NULL;

//------------------------------------------------------------------------------
// DataStore
//------------------------------------------------------------------------------

DataStore::DataStore (DataCacheInterface *pDataCache)
{
    _pDataCache = pDataCache;
}

DataStore::~DataStore()
{
}

DataStore * DataStore::getDataStore (ConfigManager *pCfgMgr, const char *pszSessionId)
{
    if (_pDataStore != NULL) {
        return _pDataStore;
    }
    if (pCfgMgr == NULL) {
        return NULL;
    }
    const String sessionId (pszSessionId);

    bool bUseDSProSettings = false;
    DataCacheInterface::StorageMode mode = DataCacheInterface::MEMORY_MODE;
    if (pCfgMgr->hasValue (PERSISTENCE_MODE)) {
        bUseDSProSettings = true;
        switch (pCfgMgr->getValueAsInt (PERSISTENCE_MODE)) {

            case DataCacheInterface::MEMORY_MODE:
                mode = DataCacheInterface::MEMORY_MODE;

            case DataCacheInterface::PERSISTENT_MODE:
                mode = DataCacheInterface::PERSISTENT_MODE;

            default:
                checkAndLogMsg ("DataStore::getDataStore", Logger::L_SevereError,
                                "non valid storage mode %d", pCfgMgr->getValueAsInt (PERSISTENCE_MODE));
                return NULL;
        }
    }

    const char *pszStorageFile = NULL;
    if (bUseDSProSettings && mode == DataCacheInterface::PERSISTENT_MODE) {
        pszStorageFile = pCfgMgr->getValue (PERSISTENCE_FILE, "db.sqlite");
    }
    const String storageFile (pszStorageFile);

    bool bUseTransactionTimer = pCfgMgr->getValueAsBool (PERSISTENCE_AUTOCOMMIT, false);

    DataCacheInterface *pDataCache = bUseDSProSettings ? DataCacheFactory::getDataCache (mode, storageFile, sessionId,
                                                                                         bUseTransactionTimer) :
                                                         DataCacheFactory::getDataCache (pCfgMgr);
    if (pDataCache == NULL) {
        checkAndLogMsg ("DataStore::getDataStore", memoryExhausted);
        return NULL;
    }

    _pDataStore = new DisServiceDataStore (pDataCache);
    if (_pDataStore == NULL) {
        checkAndLogMsg ("DataStore::getDataStore", memoryExhausted);
        return NULL;
    }

    return _pDataStore;
}

int DataStore::insert (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                       const char *pszAnnotatedObjMsgId, const void *pAnnotationMetadata, uint32 ui32AnnotationMetdataLen,
                       const char *pszMimeType, const char *pszChecksum, const char *pszReferredObjectId,
                       const void *pBuf, uint32 ui32Len, bool bIsMetadata, int64 i64ExpirationTimeout,
                       uint8 ui8ChunkId, uint8 ui8TotNChunks)
{
    if (pszId == NULL || pBuf == NULL || ui32Len == 0) {
        return -1;
    }

    if (ui8ChunkId == MessageHeader::UNDEFINED_CHUNK_ID && ui8TotNChunks > 1) {
        return -2;
    }
    if (ui8ChunkId != MessageHeader::UNDEFINED_CHUNK_ID && (ui8TotNChunks == 0 || ui8TotNChunks == 1)) {
        return -3;
    }
    if (bIsMetadata && (ui8ChunkId != MessageHeader::UNDEFINED_CHUNK_ID)) {
        // metadata messages can't be chunked
        return -4;
    }
    assert (!((pszAnnotatedObjMsgId != NULL) && (pszReferredObjectId != NULL))); // only one of the two can be not null
                                                                                 // (it's either a metadata or an annotation)

    DArray2<NOMADSUtil::String> fields (6U);
    int rc = 0;
    uint32 ui32FragmentOffset = 0;
    if (isAllChunksMessageID (pszId)) {
        rc = convertKeyToField (pszId, fields, 3, MSG_ID_GROUP, MSG_ID_SENDER,
                                MSG_ID_SEQ_NUM);
    }
    else {
        rc = convertKeyToField (pszId, fields, 5, MSG_ID_GROUP, MSG_ID_SENDER,
                                MSG_ID_SEQ_NUM, MSG_ID_CHUNK_ID, MSG_ID_OFFSET);

        ui32FragmentOffset = atoui32 (fields[MSG_ID_OFFSET]);
        if (ui32FragmentOffset != 0) {
            checkAndLogMsg ("DataStore::insert", Logger::L_MildError,
                            "trying to add a fragment - insert will fail\n");
            return -5;
        }

        uint8 ui32FragmentOffsetCheck = static_cast<uint8>(atoui32 (fields[MSG_ID_CHUNK_ID]));
        if (ui8ChunkId != ui32FragmentOffsetCheck) {
            checkAndLogMsg ("DataStore::insert", Logger::L_MildError,
                            "ui8NChunks and MSG_ID_CHUNK_ID do not correspond - insert will fail\n");
            return -6;
        }
    }

    if (rc < 0) {
        checkAndLogMsg ("DataStore::insert", Logger::L_MildError,
                        "could not convert key to fields\n");
        return -7;
    }

    MessageHeader *pMH =
            MessageHeaderHelper::getMessageHeader (fields[MSG_ID_GROUP], fields[MSG_ID_SENDER],
                                                   atoui32 (fields[MSG_ID_SEQ_NUM]),
                                                   ui8ChunkId,
                                                   pszObjectId,
                                                   pszInstanceId,
                                                   0U,    // ui16Tag
                                                   0U,    // ui16ClientId
                                                   0U,    // ui8ClientType
                                                   pszMimeType,
                                                   pszChecksum,
                                                   ui32Len,
                                                   ui32FragmentOffset,
                                                   ui32Len,
                                                   1U,    // ui32MetaDataLength,
                                                   0U,    // ui16HistoryWindow,
                                                   0U,    // ui8Priority,
                                                   i64ExpirationTimeout,
                                                   false, // bAcknowledgment,
                                                   false, // bMetaData,
                                                   ui8TotNChunks,
                                                   pszReferredObjectId);

    if (pMH == NULL) {
        checkAndLogMsg ("DataStore::insert", memoryExhausted);
        return -8;
    }

    if (pszAnnotatedObjMsgId != NULL) {
        assert (pAnnotationMetadata != NULL);
        assert (ui32AnnotationMetdataLen > 0);
        pMH->setAnnotates (pszAnnotatedObjMsgId);
        pMH->setAnnotationMetadata (pAnnotationMetadata, ui32AnnotationMetdataLen);
    }

    if ((rc = _pDataCache->addData (pMH, pBuf)) < 0) {
        checkAndLogMsg ("DataStore::insert", Logger::L_MildError,
                        "the data could not be inserted. Error code: %d\n", rc);
        delete pMH;
        return -9;
    }

    delete pMH;
    return 0;
}

NOMADSUtil::DArray<uint8> * DataStore::getCachedChunkIDs (const char *pszMsgId, uint8 &ui8TotalNumberOfChunks)
{
    ui8TotalNumberOfChunks = 0;
    if (pszMsgId == NULL || _pDataCache == NULL) {
        return NULL;
    }

    if (!isAllChunksMessageID (pszMsgId)) {
        checkAndLogMsg ("DataStore::getCachedChunkIDs", Logger::L_Warning,
                        "message id <%s> is not complete\n", pszMsgId);
        return NULL;
    }
    if (!isOnDemandDataID (pszMsgId)) {
        checkAndLogMsg ("DataStore::getCachedChunkIDs", Logger::L_Warning,
                        "message id <%s> is not on-demand, therefore it can't be chunked\n",
                        pszMsgId);
        return NULL;
    }

    DArray2<NOMADSUtil::String> fields (3U);
    int rc = convertKeyToField (pszMsgId, fields, 3, MSG_ID_GROUP, MSG_ID_SENDER,
                                MSG_ID_SEQ_NUM);
    if (rc < 0) {
        checkAndLogMsg ("DataStore::getCachedChunkIDs", Logger::L_Warning,
                        "could not parse message Id <%s>\n", pszMsgId);
    }

    DArray<uint8> *pChunkIds = new DArray<uint8>();
    if (pChunkIds == NULL) {
        checkAndLogMsg ("DataStore::getCachedChunkIDs", memoryExhausted);
        return NULL;
    }

    PtrLList<Message> *pList = _pDataCache->getCompleteMessages (fields[MSG_ID_GROUP],
                                                                 fields[MSG_ID_SENDER],
                                                                 atoui32 (fields[MSG_ID_SEQ_NUM].c_str()));
    if (pList == NULL) {
        checkAndLogMsg ("DataStore::getCachedChunkIDs", Logger::L_Warning,
                        "could not find any complete message with id <%s>\n", pszMsgId);
        return NULL;
    }
    Message *pMsg = pList->getFirst();
    if (pMsg == NULL) {
        checkAndLogMsg ("DataStore::getCachedChunkIDs", Logger::L_Warning,
                        "could not find any complete message with id <%s>\n", pszMsgId);
        _pDataCache->release (pList);
        return NULL;
    }

    for (unsigned int i = 0; pMsg != NULL; i++) {
        ChunkMsgInfo *pCMI = pMsg->getChunkMsgInfo();
        if (pCMI != NULL) {
            ui8TotalNumberOfChunks = pCMI->getTotalNumberOfChunks();
            (*pChunkIds)[i] = pCMI->getChunkId();
        }
        pMsg = pList->getNext();
    }

    _pDataCache->release (pList);
    return pChunkIds;
}

Message * DataStore::getCompleteMessage (const char *pszMsgId)
{
    if (pszMsgId == NULL || _pDataCache == NULL) {
        return NULL;
    }

    if (!isAllChunksMessageID (pszMsgId)) {
        checkAndLogMsg ("DataStore::getCompleteMessage", Logger::L_Warning,
                        "message id <%s> is not complete\n", pszMsgId);
        return NULL;
    }
    if (isOnDemandDataID (pszMsgId)) {
        checkAndLogMsg ("DataStore::getCompleteMessage", Logger::L_Warning,
                        "message id <%s> is on-demand, this method should be used "
                        "only for non chunked messages\n", pszMsgId);
        return NULL;
    }

    DArray2<NOMADSUtil::String> fields (3U);
    int rc = convertKeyToField (pszMsgId, fields, 3, MSG_ID_GROUP, MSG_ID_SENDER,
                                MSG_ID_SEQ_NUM);
    if (rc < 0) {
        checkAndLogMsg ("DataStore::getCompleteMessage", Logger::L_Warning,
                        "could not parse message Id <%s>\n", pszMsgId);
    }

    PtrLList<Message> *pList = _pDataCache->getCompleteMessages (fields[MSG_ID_GROUP],
                                                                 fields[MSG_ID_SENDER],
                                                                 atoui32 (fields[MSG_ID_SEQ_NUM].c_str()));
    if (pList == NULL) {
        checkAndLogMsg ("DataStore::getCompleteMessage", Logger::L_Warning,
                        "could not find any complete message with id <%s>\n", pszMsgId);
        return NULL;
    }
    Message *pMsg = pList->getFirst();
    if (pMsg == NULL) {
        checkAndLogMsg ("DataStore::getCompleteMessage", Logger::L_Warning,
                        "could not find any complete message with id <%s>\n", pszMsgId);
        _pDataCache->release (pList);
        return NULL;
    }
    if (pList->getNext() != NULL) {
        checkAndLogMsg ("DataStore::getCompleteMessage", Logger::L_Warning, "multiple message "
                        "matching id <%s> were found, when only one was expected\n", pszMsgId);
        _pDataCache->release (pList);
        return NULL;
    }

    if (pMsg->getMessageHeader() == NULL) {
        _pDataCache->release (pList);
        return NULL;
    }

    if (pMsg->getData() == NULL) {
        _pDataCache->release (pList);
        return NULL;
    }

    Message *pMsgCpy = pMsg->clone();
    if (pMsgCpy == NULL) {
        checkAndLogMsg ("DataStore::getCompleteMessage", memoryExhausted);
        _pDataCache->release (pList);
        return NULL;
    }

    _pDataCache->release (pList);
    return pMsgCpy;
}

PtrLList<Message> * DataStore::getCompleteChunks (const char *pszMsgId)
{
    if (pszMsgId == NULL || _pDataCache == NULL) {
        return NULL;
    }

    if (!isAllChunksMessageID (pszMsgId)) {
        checkAndLogMsg ("DataStore::getCompleteChunks", Logger::L_Warning,
                        "message id <%s> is not complete\n", pszMsgId);
        return NULL;
    }
    if (!isOnDemandDataID (pszMsgId)) {
        checkAndLogMsg ("DataStore::getCompleteChunks", Logger::L_Warning,
                        "message id <%s> is not on-demand, therefore it can't be chunked\n",
                        pszMsgId);
        return NULL;
    }

    DArray2<NOMADSUtil::String> fields (3U);
    int rc = convertKeyToField (pszMsgId, fields, 3, MSG_ID_GROUP, MSG_ID_SENDER,
                                MSG_ID_SEQ_NUM);
    if (rc < 0) {
        checkAndLogMsg ("DataStore::getCompleteChunks", Logger::L_Warning,
                        "could not parse message Id <%s>\n", pszMsgId);
    }

    PtrLList<Message> *pList = _pDataCache->getCompleteMessages (fields[MSG_ID_GROUP],
                                                                 fields[MSG_ID_SENDER],
                                                                 atoui32 (fields[MSG_ID_SEQ_NUM].c_str()));
    if (pList == NULL) {
        checkAndLogMsg ("DataStore::getCompleteChunks", Logger::L_Warning,
                        "could not find any complete message with id <%s>\n", pszMsgId);
        return NULL;
    }
    Message *pMsg = pList->getFirst();
    if (pMsg == NULL) {
        checkAndLogMsg ("DataStore::getCompleteChunks", Logger::L_Warning,
                        "could not find any complete message with id <%s>\n", pszMsgId);
        _pDataCache->release (pList);
        return NULL;
    }

    return pList;
}

int DataStore::releaseChunks (NOMADSUtil::PtrLList<Message> *pMessages)
{
    if (pMessages == NULL || _pDataCache == NULL) {
        return -1;
    }

    return _pDataCache->release (pMessages);
}

int DataStore::getData (const char *pszMsgId, void **ppData, uint32 &ui32DataLength)
{
    if (pszMsgId == NULL || ppData == NULL) {
        return -1;
    }
    *ppData = NULL;
    ui32DataLength = 0;

    const void *pBuf = _pDataCache->getData (pszMsgId, ui32DataLength);
    if (pBuf == NULL) {
         checkAndLogMsg ("DataStore::getData", Logger::L_Info,
                         "no data has been found in the cache %s\n", pszMsgId);
         ui32DataLength = 0;
         return 0;
    }
    if (ui32DataLength == 0) {
        _pDataCache->release (pszMsgId, (void*)pBuf);
        checkAndLogMsg ("DataStore::getData", Logger::L_Warning,
                        "data found in the cache %s, but it's length is 0.\n", pszMsgId);
        return -2;
    }
    checkAndLogMsg ("DataStore::getData", Logger::L_Info,
                    "data found in the cache %s\n", pszMsgId);

    (*ppData) = malloc (ui32DataLength);
    memcpy ((*ppData), pBuf, ui32DataLength);

    _pDataCache->release (pszMsgId, (void*)pBuf);

    return 0;
}

bool DataStore::hasData (const char *pszMsgId)
{
    if (pszMsgId == NULL) {
        return false;
    }

    if (isAllChunksMessageID (pszMsgId)) {
        if (isOnDemandDataID (pszMsgId)) {
            // Checking whether a complete message was received - I need to check
            // whether all the chunks have been received
            DArray2<String> tokenizedKey;
            if (convertKeyToField (pszMsgId, tokenizedKey, 3, MSG_ID_GROUP,
                                   MSG_ID_SENDER, MSG_ID_SEQ_NUM) < 0) {
                return false;
            }
            unsigned int uiTotNChunks = 0;
            int iNChunks = _pDataCache->countChunks (tokenizedKey[MSG_ID_GROUP].c_str(),
                                                     tokenizedKey[MSG_ID_SENDER].c_str(),
                                                     (uint32) atoi (tokenizedKey[MSG_ID_SEQ_NUM].c_str()),
                                                     uiTotNChunks);
            if (iNChunks < 0) {
                // Error
                return false;
            }
            if (uiTotNChunks == 0) {
                // No chunk has been received
                return false;
            }
            if (((unsigned int) iNChunks) == uiTotNChunks) {
                // All the chunks have already been received
                return true;
            }
            return false;
        }
        else {
            String msgId (pszMsgId);
            msgId += ":0";
            return _pDataCache->hasCompleteMessage (msgId.c_str());
        }
    }
    // Checking whether a complete (non-chunked) message was received    
    return _pDataCache->hasCompleteMessage (pszMsgId);
}

char ** DataStore::getDSProIds (const char *pszObjectId, const char *pszInstanceId)
{
    if (pszObjectId == NULL) {
        return NULL;
    }
    return _pDataCache->getDisseminationServiceIds (pszObjectId, pszInstanceId);
}

uint32 DataStore::getNextExpectedSeqId (const char *pszGroupName, const char *pszSenderId)
{
    return _pDataCache->getNextExpectedSeqId (pszGroupName, pszSenderId, 0);
}

bool DataStore::isMetadataMessageStored (const char *pszMsgId)
{
    if (pszMsgId == NULL || _pDataCache == NULL) {
        return NULL;
    }

    if (!isAllChunksMessageID (pszMsgId)) {
        checkAndLogMsg ("DataStore::getHeader", Logger::L_Warning,
                        "message id <%s> is not complete\n", pszMsgId);
        return NULL;
    }

    uint8 ui8ChunkId = MessageHeader::UNDEFINED_CHUNK_ID;
    DArray2<NOMADSUtil::String> fields (3U);
    int rc = convertKeyToField (pszMsgId, fields, 3, MSG_ID_GROUP, MSG_ID_SENDER,
                                MSG_ID_SEQ_NUM);
    if (rc < 0) {
        checkAndLogMsg ("DataStore::getHeader", Logger::L_Warning,
                        "message id <%s> could not be converted to fields \n", pszMsgId);
        return NULL;
    }

    return _pDataCache->hasCompleteMessage (fields[MSG_ID_GROUP], fields[MSG_ID_SENDER],
                                            atoui32 (fields[MSG_ID_SEQ_NUM]), ui8ChunkId);
}

PtrLList<MessageHeader> * DataStore::getMessageInfos (const char *pszSQLStatement)
{
    if (pszSQLStatement == NULL || _pDataCache == NULL) {
        return NULL;
    }

    return _pDataCache->getMessageInfos (pszSQLStatement);
}

PropertyStoreInterface * DataStore::getPropertyStore (void)
{
    return _pDataCache->getStorageInterface()->getPropertyStore();
}

void DataStore::lock (void)
{
    _pDataCache->lock();
}

void DataStore::unlock (void)
{
    _pDataCache->unlock();
}

//------------------------------------------------------------------------------
// DisServiceDataStore
//------------------------------------------------------------------------------

DisServiceDataStore::DisServiceDataStore (DataCacheInterface *pDataCache)
    : DataStore (pDataCache)
{
}

DisServiceDataStore::~DisServiceDataStore()
{
}

int DisServiceDataStore::insert (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                                 const char *pszAnnotatedObjMsgId, const void *pAnnotationMetadata, uint32 ui32AnnotationMetdataLen,
                                 const char *pszMimeType, const char *pszChecksum, const char *pszReferredObjectId,
                                 const void *pBuf, uint32 ui32Len, bool bIsMetadata, int64 i64ExpirationTimeout,
                                 uint8 ui8NChunks, uint8 ui8TotNChunks)
{
    if (pszId == NULL || pBuf == NULL || ui32Len == 0) {
        return -1;
    }
    if (ui8NChunks == MessageHeader::UNDEFINED_CHUNK_ID && ui8TotNChunks > 1) {
        return -2;
    }
    if (ui8NChunks != MessageHeader::UNDEFINED_CHUNK_ID && (ui8TotNChunks == 0 || ui8TotNChunks == 1)) {
        return -3;
    }

    uint32 ui32NewLen = ui32Len;
    void *pNewData = NULL;
    if (ui8NChunks == MessageHeader::UNDEFINED_CHUNK_ID) {
        // Chunks do not need to be added the header
        ui32NewLen = 0;
        pNewData = MessageHeaders::addDSProHeader (pBuf, ui32Len, bIsMetadata, ui32NewLen);
        if (pNewData == NULL) {
            return -4;
        }
    }

    int rc = DataStore::insert (pszId, pszObjectId, pszInstanceId, pszAnnotatedObjMsgId, pAnnotationMetadata,
                                ui32AnnotationMetdataLen, pszMimeType, pszChecksum, pszReferredObjectId,
                                (pNewData == NULL ? pBuf : pNewData), ui32NewLen, bIsMetadata, i64ExpirationTimeout,
                                ui8NChunks, ui8TotNChunks);
    if (pNewData != NULL) {
        free (pNewData);
    }

    return (rc == 0 ? 0 : -5);
}

int DisServiceDataStore::getNumberOfReceivedChunks (const char *pszId, uint8 &ui8NumberOfChunks,
                                                    uint8 &ui8TotalNumberOfChunks)
{
    ui8NumberOfChunks = ui8TotalNumberOfChunks = 0;

    if (pszId == NULL) {
        return -1;
    }
 
    const char *pszMethodName = "DisServiceDataStore::getNumberOfReceivedChunks";

    DArray2<NOMADSUtil::String> fields;
    if (convertKeyToField (pszId, fields, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not parse id %s\n", pszId);
        return -2;
    }

    if (!isOnDemandGroupName (fields[MSG_ID_GROUP].c_str())) {
        // The message was not chunked - just check whether it was received
        if (_pDataCache->hasCompleteMessage (fields[MSG_ID_GROUP], fields[MSG_ID_SENDER],
                                             atoui32 (fields[MSG_ID_SEQ_NUM]),
                                             MessageHeader::UNDEFINED_CHUNK_ID)) {
            ui8NumberOfChunks = 1;
        }
        ui8TotalNumberOfChunks = 1;
        return 0;
    }

    unsigned int uiTotNChunks = 0;
    int iNChunks = _pDataCache->countChunks (fields[MSG_ID_GROUP].c_str(),
                                             fields[MSG_ID_SENDER].c_str(),
                                             atoui32 (fields[MSG_ID_SEQ_NUM]),
                                             uiTotNChunks);
    if ((iNChunks < 0) || (iNChunks > 0xFF) || (uiTotNChunks > 0xFF)) {
        iNChunks = 0;
        ui8TotalNumberOfChunks = 0;
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "countChunks() returned %d\n", iNChunks);
        return -3;
    }

    ui8TotalNumberOfChunks = (uint8) uiTotNChunks;
    ui8NumberOfChunks = (uint8) iNChunks;
    return 0;
}

int DisServiceDataStore::getReceivedChunkIds (const char *pszId, DArray<uint8> &receivedChunkIds,
                                              uint8 &ui8TotalNumberOfChunks)
{
    ui8TotalNumberOfChunks = 0;

    if (pszId == NULL) {
        return -1;
    }
 
    const char *pszMethodName = "DisServiceDataStore::getReceivedChunkIds";

    DArray2<NOMADSUtil::String> fields;
    if (convertKeyToField (pszId, fields, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not parse id %s\n", pszId);
        return -2;
    }

    uint32 ui32MsgSeqId = atoui32 (fields[MSG_ID_SEQ_NUM]);
    if (!isOnDemandGroupName (fields[MSG_ID_GROUP].c_str())) {
        // The message was not chunked - just check whether it was received
        if (_pDataCache->hasCompleteMessage (fields[MSG_ID_GROUP], fields[MSG_ID_SENDER],
                                             ui32MsgSeqId, MessageHeader::UNDEFINED_CHUNK_ID)) {
            receivedChunkIds[0] = MessageHeader::UNDEFINED_CHUNK_ID;
        }
        ui8TotalNumberOfChunks = 1;
        return 0;
    }

    PtrLList<MessageHeader> *pChunks = _pDataCache->getCompleteChunkMessageInfos ((const char *) fields[MSG_ID_GROUP],
                                                                                  (const char *) fields[MSG_ID_SENDER],
                                                                                  ui32MsgSeqId);
    if (pChunks != NULL) {
        MessageHeader *pNext = pChunks->getFirst();
        MessageHeader *pCurr;
        for (unsigned int i = 0; (pCurr = pNext) != NULL; i++) {
            if (i == 0) {
                ui8TotalNumberOfChunks = pCurr->getTotalNumberOfChunks();
            }
            pNext = pChunks->getNext();
            receivedChunkIds[i] = pCurr->getChunkId();
            delete pChunks->remove (pCurr);
        }
        delete pChunks;
    }

    return 0;
}

