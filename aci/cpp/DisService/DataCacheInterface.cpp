/*
 * DataCacheInterface.cpp
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

#include "DataCacheInterface.h"

#include "DataCache.h"
#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "DSSFLib.h"
#include "Logger.h"
#include "Message.h"
#include "MessageInfo.h"
#include "PersistentDataCache.h"

#include "ConfigManager.h"
#include "File.h"
#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const bool DataCacheInterface::DEFAULT_IS_NOT_TARGET = false;

DataCacheInterface::DataCacheInterface()
{
    _ui32CacheLimit = DEFAULT_MAX_CACHE_SIZE;
    _secRange = DEFAULT_CACHE_SECURITY_THREASHOLD;
    _ui32CurrentCacheSize = 0;
}

DataCacheInterface::~DataCacheInterface()
{
    delete _pDB;
    _pDB = NULL;
}

int DataCacheInterface::deregisterAllDataCacheListeners()
{
    return _notifier.deregisterAllListeners();
}

int DataCacheInterface::deregisterDataCacheListener (unsigned int uiIndex)
{
    return _notifier.deregisterListener (uiIndex);
}

int DataCacheInterface::registerDataCacheListener (DataCacheListener *pListener)
{
    return _notifier.registerListener (pListener);
}

int DataCacheInterface::addData (MessageHeader *pMessageHeader, const void *pData)
{
    return addDataNoNotify (pMessageHeader, pData, Notifier::ID_UNSET);
}

int DataCacheInterface::addData (MessageHeader *pMessageHeader, const char *pszFilePath)
{
    _m.lock (33);
    // TODO: implement this!
    exit (-1);
    _m.unlock (33);
    return 0;
}

int DataCacheInterface::addDataNoNotify (MessageHeader *pMessageHeader, const void *pData,
                                         unsigned int uiListenerID)
{
    _m.lock (34);
    int rc = addDataNoNotifyInternal (pMessageHeader, pData, uiListenerID);
    _m.unlock (34);

    if (rc == 0) {
        if (uiListenerID != Notifier::ID_UNSET) {
            _notifier.dataCacheUpdatedNoNotify (pMessageHeader, pData, uiListenerID);
        }
        else {
            _notifier.dataCacheUpdated (pMessageHeader, pData);
        }
    }
    return rc;
}

void DataCacheInterface::cacheCleanCycle()
{
    // Lock the data cache during the clean cycle
    _notifier.cacheCleanCycle();
}

void DataCacheInterface::setCacheLimit (uint32 ui32CacheLimit)
{
    _ui32CacheLimit = ui32CacheLimit;
}

void DataCacheInterface::setSecurityRangeSize (uint32 ui32SecRange)
{
    _secRange = ui32SecRange;
}

bool DataCacheInterface::hasCompleteMessage (const char *pszId)
{
    _m.lock (36);
    bool bRet = _pDB->hasCompleteMessage (pszId);
    _m.unlock (36);
    return bRet;
}

bool DataCacheInterface::hasCompleteMessage (const char *pszGroupName, const char *pszSenderNodeID,
                                             uint32 ui32MsgSeqId, uint8 ui8ChunkId)
{
    _m.lock (37);
    bool bRet = _pDB->hasCompleteMessage (pszGroupName, pszSenderNodeID,
                                          ui32MsgSeqId, ui8ChunkId);
    _m.unlock (37);
    return bRet;
}

bool DataCacheInterface::hasCompleteMessage (MessageHeader *pMH)
{
    _m.lock (38);
    bool bRet = _pDB->hasCompleteMessage (pMH);
    _m.unlock (38);
    return bRet;
}

bool DataCacheInterface::hasCompleteMessageOrAnyCompleteChunk (const char *pszId)
{
    if (pszId == NULL) {
        return false;
    }
    DArray2<String> tokenizedKey (3);
    convertKeyToField (pszId, tokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM);
    return hasCompleteMessageOrAnyCompleteChunk (tokenizedKey[MSG_ID_GROUP].c_str(), tokenizedKey[MSG_ID_SENDER].c_str(), atoui32 (tokenizedKey[MSG_ID_SEQ_NUM].c_str()));
}

bool DataCacheInterface::hasCompleteMessageOrAnyCompleteChunk (const char *pszGroupName, const char *pszPublisherNodeId,
                                           uint32 ui32MsgSeqId)
{
    _m.lock (38);
    bool bRet = _pDB->hasCompleteMessageOrAnyCompleteChunk (pszGroupName, pszPublisherNodeId,
                                                            ui32MsgSeqId);
    _m.unlock (38);
    return bRet;
}

MessageHeader * DataCacheInterface::getMessageInfo (const char *pszId)
{
    _m.lock (39);
    MessageHeader *pRet = _pDB->getMsgInfo (pszId);
    _m.unlock (39);
    return pRet;
}

bool DataCacheInterface::containsMessage (MessageHeader *pMH)
{
    _m.lock (40);
    MessageHeader *pRet = _pDB->getMsgInfo (pMH->getMsgId());
    bool bRet = (pRet != NULL);
    delete pRet;
    _m.unlock (40);
    return bRet;
}

int DataCacheInterface::countChunks (const char *pszGroupName, const char *pszSenderNodeID,
                                    uint32 ui32MsgSeqId, unsigned int &uiTotNChunks)
{
    _m.lock (41);
    int rc =_pDB->countChunks (pszGroupName, pszSenderNodeID, ui32MsgSeqId, uiTotNChunks);
    _m.unlock (41);
    return rc;
}

PtrLList<MessageHeader> * DataCacheInterface::getMessageInfos (DisServiceDataCacheQuery *pQuery)
{
    PtrLList<MessageHeader> *pMIL;
    #if defined (USE_SQLITE)
       pMIL = getMessageInfos (pQuery->getSqlQuery());
    #else
       pMIL = _pDB->getMsgInfo (pQuery);
    #endif

    if ((pMIL != NULL) && (pMIL->getFirst() == NULL)) {
        delete pMIL;
        pMIL = NULL;
    }

    return pMIL;
}

DArray2<NOMADSUtil::String> * DataCacheInterface::getMessageIDs (const char *pszSQLStatement)
{
    DArray2<String> *pRet = _pDB->execSelectID (pszSQLStatement);
    return pRet;
}

DArray2<String> * DataCacheInterface::getSenderNodeIds (const char *pszGroupName)
{
    return _pDB->getSenders (pszGroupName, 0);
}

PtrLList<StorageInterface::RetrievedSubscription> * DataCacheInterface::getSubscriptions (const char *pszSenderNodeId)
{
    return _pDB->retrieveSubscriptionGroups (pszSenderNodeId);
}

uint32 DataCacheInterface::getNextExpectedSeqId (const char *pszGroupName, const char *pszSenderId, uint16 ui16Tag)
{
    return _pDB->getNextExpectedSeqId (pszGroupName, pszSenderId, ui16Tag);
}

PtrLList<MessageHeader> * DataCacheInterface::getMessageInfos (const char *pszSQLStatement)
{
    PtrLList<MessageHeader> *pMIL = _pDB->execSelectMsgInfo (pszSQLStatement, 0);
    if (pMIL && (pMIL->getFirst() == NULL)) {
        delete pMIL;
        pMIL = NULL;
    }

    return pMIL;
}

NOMADSUtil::PtrLList<MessageId> * DataCacheInterface::getNotReplicatedMsgList (const char *pszTargetPeer, unsigned int uiLimit,
                                                                               NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pFilters)
{
    PtrLList<MessageId> *pMIL = _pDB->getNotReplicatedMsgList (pszTargetPeer, uiLimit, pFilters);
    if (pMIL && (pMIL->getFirst() == NULL)) {
        delete pMIL;
        pMIL = NULL;
    }

    return pMIL;
}

NOMADSUtil::PtrLList<MessageHeader> * DataCacheInterface::getCompleteChunkMessageInfos (const char *pszMsgId)
{
    if (pszMsgId == NULL) {
        return NULL;
    }
    DArray2<String> aTokenizedKey (3);
    if (convertKeyToField (pszMsgId, aTokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM) < 0) {
        return NULL;
    }
    return getCompleteChunkMessageInfos (aTokenizedKey[MSG_ID_GROUP], aTokenizedKey[MSG_ID_SENDER],
                                         atoui32 (aTokenizedKey[MSG_ID_SEQ_NUM]));
}

PtrLList<MessageHeader> * DataCacheInterface::getCompleteChunkMessageInfos (const char *pszGroupName, const char *pszSender,
                                                                            uint32 ui32MsgSeqId)
{
    PtrLList<MessageHeader> *pMIL = _pDB->getCompleteChunkMessageInfos (pszGroupName, pszSender, ui32MsgSeqId);
    if (pMIL && (pMIL->getFirst() == NULL)) {
        delete pMIL;
        pMIL = NULL;
    }

    return pMIL;
}

PtrLList<Message> * DataCacheInterface::getMatchingFragments (const char *pszGroupName, const char *pszSenderNodeId,
                                                              uint32 ui32MsgSeqId)
{
    PtrLList<MessageHeader> *pMIs = _pDB->getMatchingFragments (pszGroupName, pszSenderNodeId, ui32MsgSeqId);
    PtrLList<Message> *pRet = getMatchingFragments (pMIs); // the elements have been moved elsewhere by getMatchingFragments()
    delete pMIs;
    return pRet;
}

PtrLList<Message> * DataCacheInterface::getMatchingFragments (const char *pszGroupName, const char *pszPublisherNodeId,
                                                              uint32 ui32MsgSeqId, uint8 ui8ChunkId)
{
    PtrLList<MessageHeader> *pMIs = _pDB->getMatchingFragments (pszGroupName, pszPublisherNodeId, ui32MsgSeqId,
                                                                ui8ChunkId);
    PtrLList<Message> *pRet = getMatchingFragments (pMIs); // the elements have been moved elsewhere by getMatchingFragments()
    delete pMIs;
    return pRet;
}

PtrLList<Message> * DataCacheInterface::getMatchingFragments (const char *pszGroupName, const char *pszPublisherNodeId,
                                                              uint32 ui32MsgSeqId, uint8 ui8ChunkId, uint32 ui32StartOffset,
                                                              uint32 ui32EndOffset)
{
    PtrLList<MessageHeader> *pMIs = _pDB->getMatchingFragments (pszGroupName, pszPublisherNodeId, ui32MsgSeqId,
                                                                ui8ChunkId, ui32StartOffset, ui32EndOffset);
    PtrLList<Message> *pRet = getMatchingFragments (pMIs); // the elements have been moved elsewhere by getMatchingFragments()
    delete pMIs;
    return pRet;
}

PtrLList<Message> * DataCacheInterface::getMatchingFragments (PtrLList<MessageHeader> *pMIs)
{
    if (pMIs == NULL) {
        return NULL;
    }
    MessageHeader *pMI = pMIs->getFirst();
    if (pMIs == NULL) {
        delete pMIs;
        return NULL;
    }
    PtrLList<Message> *pFragments = new PtrLList<Message>();
    if (pFragments == NULL) {
        MessageHeader *pMITmp = pMIs->getFirst();
        while ((pMI = pMITmp) != NULL) {
            pMITmp = pMIs->getNext();
            delete pMIs->remove (pMI);
        }
        checkAndLogMsg ("DataCacheInterface::getMatchingFragments", memoryExhausted);
        return NULL;
    }

    // If pMI has already been initialized and if it were NULL, it would have
    // already returned
    Message *pMsg;
    const void *pData;
    do {
        pData = getData (pMI->getMsgId());
        if (pData != NULL) {
            pMsg = new Message (pMI, pData);
            if (pMsg != NULL) {
                pFragments->append (pMsg);
            }
            else {
                checkAndLogMsg ("DataCacheInterface::getMatchingFragments", memoryExhausted);
                break;
            }
        }
        else {
            checkAndLogMsg ("DataCacheInterface::getMatchingFragments", Logger::L_MildError,
                            "it was found the message info for %s, but not the data\n",
                            pMI->getMsgId());
        }
    } while ((pMI = pMIs->getNext()) != NULL);

    return pFragments;
}

PtrLList<MessageHeader> * DataCacheInterface::getMessageInfos (const char *pszGroupName, const char *pszSenderNodeId,
                                                               uint8 ui8ClientType, uint16 ui6Tag,
                                                               uint32 ui32From, uint32 ui32To)
{
    PtrLList<MessageHeader> *pMIL = _pDB->execSelectMsgInfo (pszGroupName, pszSenderNodeId, ui8ClientType,
                                                             ui6Tag, ui32From, ui32To, 0, 0);
    if (pMIL && (pMIL->getFirst() == NULL)) {
        delete pMIL;
        pMIL = NULL;
    }
    return pMIL;
}

char ** DataCacheInterface::getDisseminationServiceIds (const char *pszObjectId, const char *pszInstanceId)
{
    return _pDB->getDisseminationServiceIds (pszObjectId, pszInstanceId);
}

void * DataCacheInterface::getAnnotationMetadata (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId, uint32 &ui32Len)
{
    return _pDB->getAnnotationMetadata (pszGroupName, pszSenderNodeId, ui32MsgSeqId, ui32Len);
}

bool DataCacheInterface::cleanCache (uint32 ui32Length, MessageHeader *pMH, void *pData)
{
    _m.lock (30);
    // Make a copy of the shared variables, so that the rest of the code does
    // not need to be mutually exclusively accessed (it is a good idea to unlock
    // the object before calling listeners)
    uint32 ui32CacheSizeTmp = _ui32CurrentCacheSize;
    uint32 ui32CacheLimitTmp = _ui32CacheLimit;
    uint32 ui32SecRangeTmp = _secRange;
    _m.unlock (30);

    // The cache has no limit constraint
    if (ui32CacheLimitTmp == 0) {
        return true;
    }

    if ((ui32CacheSizeTmp + ui32Length) >= (ui32CacheLimitTmp - ui32SecRangeTmp)) {
        // if there is an amount of data close to the limit
    	_notifier.thresholdCapacityReached (ui32Length);
    }
    else {
        return true;
    }

    // if still, there's not room enough
    if ((ui32CacheSizeTmp + ui32Length) >= ui32CacheLimitTmp) {
        // this means that the cache is too small to contain the data
        checkAndLogMsg ("DataCache::cleanCache", Logger::L_Warning,
                        "cache too small (%d), needed: %d\n",
        	        ui32CacheLimitTmp, ui32Length);
    	_notifier.spaceNeeded (ui32CacheSizeTmp + ui32Length - ui32CacheLimitTmp, pMH, pData);
        return false;
    }

    return true;
}

//==============================================================================
//  Result
//==============================================================================
DataCacheInterface::Result::Result()
{
    ui8StorageType = NOSTORAGETYPE;
    ui32Length = 0;
    pData = NULL;
}

DataCacheInterface::Result::~Result()
{    
}

////////////////////////////// DataCacheFactory ////////////////////////////////

DataCacheInterface * DataCacheFactory::_pDataCache = NULL;

DataCacheInterface * DataCacheFactory::getDataCache (ConfigManager *pCfgMgr)
{
    if (_pDataCache != NULL) {
        return _pDataCache;
    }

    DataCacheInterface::StorageMode mode;
    switch (pCfgMgr->getValueAsInt ("aci.disService.storageMode", DataCacheInterface::MEMORY_MODE)) {
        case DataCacheInterface::MEMORY_MODE:
            mode = DataCacheInterface::MEMORY_MODE;
            break;

        case DataCacheInterface::PERSISTENT_MODE:
            mode = DataCacheInterface::PERSISTENT_MODE;
            break;

        default:
            mode = DataCacheInterface::MEMORY_MODE;
    }

    String storageFile (pCfgMgr->getValue ("aci.disService.storageFile"));
    storageFile.trim();
    String sessionId (pCfgMgr->getValue ("aci.disService.sessionKey"));
    sessionId.trim();
    const bool bUseTransactionTimer = pCfgMgr->getValueAsBool ("aci.disService.storage.useTransactionTimer", false);

    return getDataCache (mode, storageFile, sessionId, bUseTransactionTimer);
}

DataCacheInterface * DataCacheFactory::getDataCache (DataCacheInterface::StorageMode mode,
                                                     const String &storageDBName, const String &sessionId,
                                                     bool bUseTransactionTimer)
{
    if (_pDataCache != NULL) {
        return _pDataCache;
    }

    const char *pszMethodName = "DataCacheFactory::getDataCache";
    switch (mode) {

        case DataCacheInterface::MEMORY_MODE: {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Data Cache running in MEMORY_MODE\n");
            _pDataCache = new DataCache();
            break;
        }

        case DataCacheInterface::PERSISTENT_MODE: {
            String dbName (generateDatabaseName (mode, storageDBName, sessionId));
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Data Cache running in PERSISTENT_MODE: "
                            "database name: %s\n", dbName.c_str());
            _pDataCache =  new PersistentDataCache (dbName, bUseTransactionTimer);
            break;
        }

        default:
            _pDataCache = new DataCache();
    }

    return _pDataCache;
}

String DataCacheFactory::generateDatabaseName (DataCacheInterface::StorageMode mode,
                                               const NOMADSUtil::String &storageDBName,
                                               const NOMADSUtil::String &sessionId)
{
    if (mode == DataCacheInterface::MEMORY_MODE) {
        String emptryString;
        return emptryString;
    }

    // Create file name
    const File file (storageDBName);
    String sessionSpecificstorageDBName (file.getName (true));
    if (sessionSpecificstorageDBName.length() <= 0) {
        sessionSpecificstorageDBName = "db";
    }
    if ((sessionId.length() > 0)) {
        sessionSpecificstorageDBName += "-";
        sessionSpecificstorageDBName += sessionId;
    }

    // Add extension
    const String extension (file.getExtension());
    sessionSpecificstorageDBName += '.';
    sessionSpecificstorageDBName += (extension.length() > 0) ? extension : "sqlite";

    // Add parent path
    String fullPath (file.getParent());
    if ((fullPath.length() > 0) && (!fullPath.endsWith (getPathSepCharAsString()))) {
        fullPath += getPathSepChar();
    }

    fullPath += sessionSpecificstorageDBName;
    return fullPath;
}

