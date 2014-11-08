/*
 * SQLMessageHeaderStorage.cpp
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

#include "SQLMessageHeaderStorage.h"

#include "Database.h"
#include "DisServiceDefs.h"
#include "DisServiceDataCacheQuery.h"
#include "DSSFLib.h"
#include "Message.h"
#include "MessageId.h"
#include "MessageInfo.h"
#include "PreparedStatement.h"
#include "SQLiteFactory.h"
#include "SQLPropertyStore.h"
#include "SQLTransmissionHistory.h"

#include "Logger.h"
#include "NLFLib.h"
#include "StrClass.h"

#include <stdio.h>

using namespace IHMC_ACI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

//==========================================================================
//  TABLE NAMES AND INDEXES
//==========================================================================
const String SQLMessageHeaderStorage::TABLE_NAME = "DisServiceDataCache";
const String SQLMessageHeaderStorage::CHUNK_TABLE_NAME = "DisServiceChunkCache";
const String SQLMessageHeaderStorage::INDEX_GROUP = "grpIndex";
const String SQLMessageHeaderStorage::INDEX_TAG = "tagIndex";

//==========================================================================
//  COLUMN NAMES AND COLUM INDEXES
//==========================================================================
const String SQLMessageHeaderStorage::FIELD_ROW_ID = "rowid";

const String SQLMessageHeaderStorage::FIELD_GROUP_NAME = "groupName";
const uint8 SQLMessageHeaderStorage::FIELD_GROUP_NAME_COLUMN_NUMBER = 0;

const String SQLMessageHeaderStorage::FIELD_SENDER_ID = "senderNodeId";
const uint8 SQLMessageHeaderStorage::FIELD_SENDER_ID_COLUMN_NUMBER = 1;

const String SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID = "messageSeqId";
const uint8 SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID_COLUMN_NUMBER = 2;

const String SQLMessageHeaderStorage::FIELD_CHUNK_ID = "chunkId";
const uint8 SQLMessageHeaderStorage::FIELD_CHUNK_ID_COLUMN_NUMBER = 3;

const String SQLMessageHeaderStorage::FIELD_OBJECT_ID = "objectId";
const uint8 SQLMessageHeaderStorage::FIELD_OBJECT_ID_COLUMN_NUMBER = 4;

const String SQLMessageHeaderStorage::FIELD_INSTANCE_ID = "instanceId";
const uint8 SQLMessageHeaderStorage::FIELD_INSTANCE_ID_COLUMN_NUMBER = 5;

const String SQLMessageHeaderStorage::FIELD_TAG = "tag";
const uint8 SQLMessageHeaderStorage::FIELD_TAG_COLUMN_NUMBER = 6;

const String SQLMessageHeaderStorage::FIELD_CLIENT_ID = "clientId";
const uint8 SQLMessageHeaderStorage::FIELD_CLIENT_ID_COLUMN_NUMBER = 7;

const String SQLMessageHeaderStorage::FIELD_CLIENT_TYPE = "clientType";
const uint8 SQLMessageHeaderStorage::FIELD_CLIENT_TYPE_COLUMN_NUMBER = 8;

const String SQLMessageHeaderStorage::FIELD_MIME_TYPE = "mimeType";
const uint8 SQLMessageHeaderStorage::FIELD_MIME_TYPE_COLUMN_NUMBER = 9;

const String SQLMessageHeaderStorage::FIELD_CHECKSUM = "checksum";
const uint8 SQLMessageHeaderStorage::FIELD_CHECKSUM_COLUMN_NUMBER = 10;

const String SQLMessageHeaderStorage::FIELD_TOT_MSG_LENGTH = "totalMsgLength";
const uint8 SQLMessageHeaderStorage::FIELD_TOT_MSG_LENGTH_COLUMN_NUMBER = 11;

const String SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH = "fragmentLength";
const uint8 SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH_COLUMN_NUMBER = 12;

const String SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET = "fragmentOffset";
const uint8 SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET_COLUMN_NUMBER = 13;

const String SQLMessageHeaderStorage::FIELD_METADATA_LENGTH = "metadataLength";
const uint8 SQLMessageHeaderStorage::FIELD_METADATA_LENGTH_COLUMN_NUMBER = 14;

const String SQLMessageHeaderStorage::FIELD_TOT_N_CHUNKS = "totalNumberOfChunks";
const uint8 SQLMessageHeaderStorage::FIELD_TOT_N_CHUNKS_NUMBER = 15;

const String SQLMessageHeaderStorage::FIELD_HISTORY_WINDOW = "historyWindow";
const uint8 SQLMessageHeaderStorage::FIELD_HISTORY_WINDOW_COLUMN_NUMBER = 16;

const String SQLMessageHeaderStorage::FIELD_PRIORITY = "priority";
const uint8 SQLMessageHeaderStorage::FIELD_PRIORITY_COLUMN_NUMBER = 17;

const String SQLMessageHeaderStorage::FIELD_EXPIRATION = "expirationTime";
const uint8 SQLMessageHeaderStorage::FIELD_EXPIRATION_COLUMN_NUMBER = 18;

const String SQLMessageHeaderStorage::FIELD_ACKNOLEDGMENT = "acknowledgment";
const uint8 SQLMessageHeaderStorage::FIELD_ACKNOLEDGMENT_COLUMN_NUMBER = 19;

const String SQLMessageHeaderStorage::FIELD_METADATA = "metadata";
const uint8 SQLMessageHeaderStorage::FIELD_METADATA_COLUMN_NUMBER = 20;

const String SQLMessageHeaderStorage::FIELD_ARRIVAL_TIMESTAMP = "arrivalTimeStamp";
const uint8 SQLMessageHeaderStorage::FIELD_ARRIVAL_TIMESTAMP_COLUMN_NUMBER = 21;

const String SQLMessageHeaderStorage::FIELD_DATA = "data";
const uint8 SQLMessageHeaderStorage::FIELD_DATA_COLUMN_NUMBER = 22;

//==========================================================================
//  HANDY GROUPS OF COLUMNS
//==========================================================================
// NOTE: Keep the fields in METAINFO_FIELDS the same order they listed above

const String SQLMessageHeaderStorage::METAINFO_FIELDS = (String) FIELD_GROUP_NAME + ", " + FIELD_SENDER_ID + ", " + FIELD_MSG_SEQ_ID + ", "
                                                               + FIELD_CHUNK_ID + ", " + FIELD_OBJECT_ID  + ", " + FIELD_INSTANCE_ID + ", "
                                                               + FIELD_TAG + ", " + FIELD_CLIENT_ID + ", " + FIELD_CLIENT_TYPE + ", " + FIELD_MIME_TYPE + ", "
                                                               + FIELD_CHECKSUM + ", " + FIELD_TOT_MSG_LENGTH + ", " + FIELD_FRAGMENT_LENGTH + ", "
                                                               + FIELD_FRAGMENT_OFFSET + ", " + FIELD_METADATA_LENGTH + ", " + FIELD_TOT_N_CHUNKS + ", "
                                                               + FIELD_HISTORY_WINDOW + ", " + FIELD_PRIORITY + ", " + FIELD_EXPIRATION + ", "
                                                               + FIELD_ACKNOLEDGMENT + ", " + FIELD_METADATA;

const String SQLMessageHeaderStorage::ALL = (String) METAINFO_FIELDS + ", " + FIELD_ARRIVAL_TIMESTAMP;
const String SQLMessageHeaderStorage::ALL_PERSISTENT = (String) ALL + ", " + FIELD_DATA;

const String SQLMessageHeaderStorage::PRIMARY_KEY = (String) FIELD_GROUP_NAME + ", " + FIELD_SENDER_ID + ", " + FIELD_MSG_SEQ_ID + ", " +
                                                       FIELD_CHUNK_ID + ", " + FIELD_FRAGMENT_OFFSET + ", " + FIELD_FRAGMENT_LENGTH;
const uint8 SQLMessageHeaderStorage::MSG_ID_N_FIELDS = 6;

SQLMessageHeaderStorage::SQLMessageHeaderStorage()
    : StorageInterface(),
      _m (26)
{
    _pPropStore = NULL;
    _pCountChunksPrepStmt = NULL;
    _pInsertByteCode = NULL;
    _pHasCompleteMsgPrepStmt = NULL;
    _pHasCompleteMsgOrAnyChunkPrepStmt = NULL;
    _pGetMsgInfoPrepStmt = NULL;
    _pGetCompleteMsgInfoPrepStmt = NULL;
    _pGetNotSentMsgInfoPrepStmt = NULL;
    _pGetMatchingFragmentMsgInfoPrepStmt_1 = NULL;
    _pGetMatchingFragmentMsgInfoPrepStmt_2 = NULL;
    _pGetMatchingFragmentMsgInfoPrepStmt_3 = NULL;
    _pGetComplChunkMsgHeadersPrepStmt = NULL;
    _pEliminateFragments = NULL;
    _pEliminateFragmentIDs = NULL;
    _pDeleteRow = NULL;
    _pFindAllMessageIDsWithFragments = NULL;
    _pGetDSIdsByObjAndInstId = NULL;
    _pGetDSIdsByObjId = NULL;
    _pDB = NULL;
}

SQLMessageHeaderStorage::SQLMessageHeaderStorage (const char *pszDBName)
    : StorageInterface (pszDBName),
      _m (26)
{
    _pPropStore = NULL;
    _pCountChunksPrepStmt = NULL;
    _pInsertByteCode = NULL;
    _pHasCompleteMsgPrepStmt = NULL;
    _pHasCompleteMsgOrAnyChunkPrepStmt = NULL;
    _pGetMsgInfoPrepStmt = NULL;
    _pGetCompleteMsgInfoPrepStmt = NULL;
    _pGetNotSentMsgInfoPrepStmt = NULL;
    _pGetMatchingFragmentMsgInfoPrepStmt_1 = NULL;
    _pGetMatchingFragmentMsgInfoPrepStmt_2 = NULL;
    _pGetMatchingFragmentMsgInfoPrepStmt_3 = NULL;
    _pGetComplChunkMsgHeadersPrepStmt = NULL;
    _pEliminateFragments = NULL;
    _pEliminateFragmentIDs = NULL;
    _pDeleteRow = NULL;
    _pFindAllMessageIDsWithFragments = NULL;
    _pGetDSIdsByObjAndInstId = NULL;
    _pGetDSIdsByObjId = NULL;
    _pDB = NULL;
}

SQLMessageHeaderStorage::~SQLMessageHeaderStorage()
{
    _m.lock (200);
    delete _pPropStore;
    _pPropStore = NULL;
    delete _pCountChunksPrepStmt;
    _pCountChunksPrepStmt = NULL;
    delete _pInsertByteCode;
    _pInsertByteCode = NULL;
    delete _pHasCompleteMsgPrepStmt;
    _pHasCompleteMsgPrepStmt = NULL;
    delete _pHasCompleteMsgOrAnyChunkPrepStmt;
    _pHasCompleteMsgOrAnyChunkPrepStmt = NULL;
    delete _pGetMsgInfoPrepStmt;
    _pGetMsgInfoPrepStmt = NULL;
    delete _pGetCompleteMsgInfoPrepStmt;
    _pGetCompleteMsgInfoPrepStmt = NULL;
    delete _pGetNotSentMsgInfoPrepStmt;
    _pGetNotSentMsgInfoPrepStmt = NULL;
    delete _pGetMatchingFragmentMsgInfoPrepStmt_1;
    _pGetMatchingFragmentMsgInfoPrepStmt_1 = NULL;
    delete _pGetMatchingFragmentMsgInfoPrepStmt_2;
    _pGetMatchingFragmentMsgInfoPrepStmt_2 = NULL;
    delete _pGetMatchingFragmentMsgInfoPrepStmt_3;
    _pGetMatchingFragmentMsgInfoPrepStmt_3 = NULL;
    delete _pGetComplChunkMsgHeadersPrepStmt;
    _pGetComplChunkMsgHeadersPrepStmt = NULL;
    delete _pEliminateFragments;
    _pEliminateFragments = NULL;
    delete _pEliminateFragmentIDs;
    _pEliminateFragmentIDs = NULL;
    delete _pFindAllMessageIDsWithFragments;
    _pFindAllMessageIDsWithFragments = NULL;
    delete _pDeleteRow;
    _pDeleteRow = NULL;

    _pDB->close();
    _pDB = NULL;
    _m.unlock (200);
}

int SQLMessageHeaderStorage::init()
{
    const char *pszMethodName = "SQLMessageHeaderStorage::init";
    
    _m.lock (201);
    _pDB = Database::getDatabase (Database::SQLite);
    if (_pDB == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Database::getDatabase returned NULL pointer.\n");
         _m.unlock (201);
        return -1;
    }
    if (_pDB->open (_dbName.c_str()) != 0) {
         _m.unlock (201);
        return -2;
    }

    char *pszSQL = getCreateTableSQLStatement();
    if (_pDB->execute (pszSQL) < 0) {
        free (pszSQL);
        _m.unlock (201);
        return -2;
    }
    free (pszSQL);

    String index = "CREATE INDEX seqIdIdx ON ";
    index = index + TABLE_NAME;
    index = index + "(" + FIELD_MSG_SEQ_ID + ");" ;

    //--------------------------------------------------------------------------
    // INSERT
    //--------------------------------------------------------------------------

    pszSQL = getInsertIntoTableSQLStatement();
    _pInsertByteCode = _pDB->prepare (pszSQL);
    if (_pInsertByteCode == NULL) {
        free (pszSQL);
        _m.unlock (201);
        return -3;
    }
    free (pszSQL);

    // Initialize the Property Store
    _pPropStore = new SQLPropertyStore();
    int rc;
    if ((rc = _pPropStore->init (_dbName.c_str())) != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to initialize SQLPropertyStore; rc = %d\n", rc);
        _m.unlock (201);
        return -4;
    }
    _m.unlock (201);
    return 0;
}

PropertyStoreInterface * SQLMessageHeaderStorage::getPropertyStore (void)
{
    return _pPropStore;
}

int SQLMessageHeaderStorage::insert (Message *pMsg)
{
    if (pMsg == NULL) {
        return -1;
    }
    MessageHeader *pMH = pMsg->getMessageHeader();
    if (pMH == NULL) {
        return -2;
    }

    _m.lock (205);

    if (_pInsertByteCode == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insert", Logger::L_SevereError,
                        "ByteCode NULL\n");
        _m.unlock (205);
        return -3;
    }

    if (insertIntoDataCacheBind (_pInsertByteCode, pMsg) < 0) {
        _pInsertByteCode->reset();
        _m.unlock (205);
        return -4;
    }

    int rc = _pInsertByteCode->update();
    if (rc < 0) {
        const char *pszMsgId = pMH->getMsgId();
        checkAndLogMsg ("SQLMessageHeaderStorage::insert", Logger::L_SevereError,
                        "update failed with rc %d; could not insert message %s\n",
                        rc, (pszMsgId == NULL ? "" : pszMsgId));
    }

    _pInsertByteCode->reset();
    _m.unlock (205);
    return rc;
}

int SQLMessageHeaderStorage::insertIntoDataCacheBind (PreparedStatement *pStmt, Message *pMsg)
{
    MessageHeader *pMH = pMsg->getMessageHeader();
    if (pMH == NULL) {
        return -1;
    }

    uint32 ui32MetadataLength = 0;
    bool bAcknoledgemnt = false;
    bool bIsMetadata = false;
    uint8 ui8NChunks = 0;
    int64 i64ArrivalTimestamp = getTimeInMilliseconds();
    if (pMH->isChunk()) {
        ChunkMsgInfo *pCMI = (ChunkMsgInfo *) pMH;
        ui8NChunks = pCMI->getTotalNumberOfChunks();
    }
    else {
        MessageInfo *pMI = (MessageInfo *) pMH;
        ui32MetadataLength = pMI->getMetaDataLength();
        bAcknoledgemnt = pMI->getAcknowledgment();
        bIsMetadata = pMI->isMetaData();
    }

    if (pStmt->bind (FIELD_GROUP_NAME_COLUMN_NUMBER + 1, pMH->getGroupName()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -2;
    }
    if (pStmt->bind (FIELD_SENDER_ID_COLUMN_NUMBER + 1, pMH->getPublisherNodeId()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -3;
    }
    if (pStmt->bind (FIELD_MSG_SEQ_ID_COLUMN_NUMBER + 1, pMH->getMsgSeqId()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -4;
    }
    if (pStmt->bind (FIELD_CHUNK_ID_COLUMN_NUMBER + 1, pMH->getChunkId()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -5;
    }
    if (pStmt->bind (FIELD_OBJECT_ID_COLUMN_NUMBER + 1, pMH->getObjectId()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -6;
    }
    if (pStmt->bind (FIELD_INSTANCE_ID_COLUMN_NUMBER + 1, pMH->getInstanceId()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -7;
    }
    if (pStmt->bind (FIELD_TAG_COLUMN_NUMBER + 1, pMH->getTag()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -8;
    }
    if (pStmt->bind (FIELD_CLIENT_ID_COLUMN_NUMBER + 1, pMH->getClientId()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -9;
    }
    if (pStmt->bind (FIELD_CLIENT_TYPE_COLUMN_NUMBER + 1, pMH->getClientType()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -10;
    }
    if (pStmt->bind (FIELD_MIME_TYPE_COLUMN_NUMBER + 1, pMH->getMimeType()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -11;
    }
    if (pStmt->bind (FIELD_CHECKSUM_COLUMN_NUMBER + 1, pMH->getChecksum()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -12;
    }
    if (pStmt->bind (FIELD_TOT_MSG_LENGTH_COLUMN_NUMBER + 1, pMH->getTotalMessageLength()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -14;
    }
    if (pStmt->bind (FIELD_FRAGMENT_LENGTH_COLUMN_NUMBER + 1, pMH->getFragmentLength()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -15;
    }
    if (pStmt->bind (FIELD_FRAGMENT_OFFSET_COLUMN_NUMBER + 1, pMH->getFragmentOffset()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -16;
    }
    if (pStmt->bind (FIELD_METADATA_LENGTH_COLUMN_NUMBER + 1, ui32MetadataLength) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -17;
    }
    if (pStmt->bind (FIELD_TOT_N_CHUNKS_NUMBER + 1, ui8NChunks) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -18;
    }
    if (pStmt->bind (FIELD_HISTORY_WINDOW_COLUMN_NUMBER + 1, pMH->getHistoryWindow()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -19;
    }
    if (pStmt->bind (FIELD_PRIORITY_COLUMN_NUMBER + 1, pMH->getPriority()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -20;
    }
    if (pStmt->bind (FIELD_EXPIRATION_COLUMN_NUMBER + 1, pMH->getExpiration()) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -21;
    }
    if (pStmt->bind (FIELD_ACKNOLEDGMENT_COLUMN_NUMBER + 1, bAcknoledgemnt) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -22;
    }
    if (pStmt->bind (FIELD_METADATA_COLUMN_NUMBER + 1, bIsMetadata) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -2;
    }
    if (pStmt->bind (FIELD_ARRIVAL_TIMESTAMP_COLUMN_NUMBER + 1, i64ArrivalTimestamp) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::insertIntoDataCacheBind", bindingError);
        return -2;
    }

    return 0;
}

bool SQLMessageHeaderStorage::hasCompleteMessage (const char *pszKey)
{
    if (pszKey == NULL) {
        return false;
    }

    uint8 ui8ChunkId = MessageHeader::UNDEFINED_CHUNK_ID;
    DArray2<String> tokenizedKey (4);
    if (isAllChunksMessageID (pszKey)) {
        convertKeyToField (pszKey, tokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM);
    }
    else {
        convertKeyToField (pszKey, tokenizedKey, 4, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM, MSG_ID_CHUNK_ID);
        ui8ChunkId = (uint8) atoi (tokenizedKey[MSG_ID_CHUNK_ID]);
    }

    return hasCompleteMessage ((const char *) tokenizedKey[MSG_ID_GROUP],
                               (const char *) tokenizedKey[MSG_ID_SENDER],
                               atoui32 (tokenizedKey[MSG_ID_SEQ_NUM]),
                               ui8ChunkId);
}

bool SQLMessageHeaderStorage::hasCompleteMessage (const char *pszGroupName, const char *pszSenderNodeID,
                                                  uint32 ui32MsgSeqId, uint8 ui8ChunkSeqId)
{
    return hasCompleteDataMessage (pszGroupName, pszSenderNodeID, ui32MsgSeqId, ui8ChunkSeqId);
}

bool SQLMessageHeaderStorage::hasCompleteMessage (MessageHeader *pMH)
{
    return hasCompleteDataMessage (pMH->getGroupName(), pMH->getPublisherNodeId(),
                                   pMH->getMsgSeqId(), pMH->getChunkId());
}

bool SQLMessageHeaderStorage::hasCompleteMessageOrAnyCompleteChunk (const char *pszGroupName, const char *pszSenderNodeId,
                                                                    uint32 ui32MsgSeqId)
{
    return hasCompleteDataMessage (pszGroupName, pszSenderNodeId, ui32MsgSeqId,
                                   MessageHeader::UNDEFINED_CHUNK_ID, false);
}

bool SQLMessageHeaderStorage::hasCompleteDataMessage (const char *pszGroupName, const char *pszSenderNodeId,
                                                      uint32 ui32MsgSeqId, uint8 ui8ChunkId,
                                                      bool bUseConstraintOnChunkId)
{
    const char *pszMethodName = "SQLMessageHeaderStorage::hasCompleteDataMessage";

    _m.lock (206);
    PreparedStatement *pStmt = NULL;
    if (bUseConstraintOnChunkId) {
        if (_pHasCompleteMsgPrepStmt == NULL) {
            // Create the prepared statement if it does not already exist
            String sql = (String) "SELECT COUNT (*) FROM " + TABLE_NAME + " WHERE "
                       + FIELD_GROUP_NAME + " = ?1 AND "
                       + FIELD_SENDER_ID + " = ?2 AND "
                       + FIELD_MSG_SEQ_ID + " = ?3 AND "
                       + FIELD_CHUNK_ID + " = ?4 AND "
                       + FIELD_FRAGMENT_OFFSET +" = 0 AND "
                       + FIELD_FRAGMENT_LENGTH + " = " + FIELD_TOT_MSG_LENGTH + ";";

            _pHasCompleteMsgPrepStmt = _pDB->prepare (sql.c_str());
            if (_pHasCompleteMsgPrepStmt != NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_Info,
                                "Statement %s prepared successfully.\n", (const char *)sql);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "Could not prepare statement: %s.\n",
                                (const char *)sql);
                _m.unlock (206);
                return false;
            }
        }
        pStmt = _pHasCompleteMsgPrepStmt;
    }
    else {
        if (_pHasCompleteMsgOrAnyChunkPrepStmt == NULL) {
            String sql = (String) "SELECT COUNT (*) FROM " + TABLE_NAME + " WHERE "
                       + FIELD_GROUP_NAME + " = ?1 AND "
                       + FIELD_SENDER_ID + " = ?2 AND "
                       + FIELD_MSG_SEQ_ID + " = ?3 AND "
                       + FIELD_FRAGMENT_OFFSET +" = 0 AND "
                       + FIELD_FRAGMENT_LENGTH + " = " + FIELD_TOT_MSG_LENGTH + ";";

            _pHasCompleteMsgOrAnyChunkPrepStmt = _pDB->prepare (sql.c_str());
            if (_pHasCompleteMsgOrAnyChunkPrepStmt != NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_Info,
                                "Statement %s prepared successfully.\n", (const char *)sql);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "Could not prepare statement: %s.\n",
                                (const char *)sql);
                _m.unlock (206);
                return false;
            }
        }
        pStmt = _pHasCompleteMsgOrAnyChunkPrepStmt;
    }

    // Bind the values to the prepared statement
    pStmt->reset();
    if (pStmt->bind (1, pszGroupName) < 0 ||
        pStmt->bind (2, pszSenderNodeId) < 0 ||
        pStmt->bind (3, ui32MsgSeqId) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Error when binding values.\n");
        _m.unlock (206);
        return false;
    }
    if (bUseConstraintOnChunkId && (pStmt->bind (4, ui8ChunkId) < 0)) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Error when binding values.\n");
        _m.unlock (206);
        return false;
    }

    // Execute the statement
    Row *pRow = pStmt->getRow();
    bool rc = pStmt->next (pRow);
    int iCount = 0;
    rc = (pRow->getValue (0, iCount) == 0);
    delete pRow;
    _m.unlock (206);

    if (!rc) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "No element was returned for parameters. "
                        "The parameters were: %s,%s,%u,%u.\n",
                        pszGroupName, pszSenderNodeId, ui32MsgSeqId, ui8ChunkId);
    }

    return (iCount > 0);
}

PtrLList<MessageHeader> * SQLMessageHeaderStorage::getMsgInfo (DisServiceDataCacheQuery *pQuery)
{
    uint16 ui16Limit;

    if (pQuery->isLimitSet()) {
        ui16Limit = pQuery->getLimit();
    }
    else {
        ui16Limit = 65535;
    }

    _m.lock (207);

    PreparedStatement *pPrepStmt = _pDB->prepare (pQuery->getSqlQuery());
    if (pPrepStmt == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMsgInfo", Logger::L_SevereError,
                        "error preparing statement\n");
        _m.unlock (207);
        return NULL;
    }

    Row *pRow = pPrepStmt->getRow();
    if (pRow == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMsgInfo", Logger::L_SevereError,
                        "could not instantiate row\n");
        delete pPrepStmt;
        _m.unlock (207);
        return NULL;
    }

    if (!pPrepStmt->next (pRow)) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMsgInfo", Logger::L_Info, "query %s did not return any result\n",
                        pQuery->getSqlQuery());
        delete pRow;
        delete pPrepStmt;
        _m.unlock (207);
        return NULL;
    }

    uint16 ui16Count = 0;
    PtrLList<MessageHeader> *pRet = new PtrLList<MessageHeader>();
    do {
        const char *pszMsgId = getId (pRow);
        if (pszMsgId == NULL) {
            break;
        }

        MessageHeader *pMI = getMsgInfo (pszMsgId);
        checkAndLogMsg ("SQLMessageHeaderStorage::getMsgInfo", Logger::L_HighDetailDebug,
                        "Calling getMsgInfo for key <%s> returned pointer to %p\n",
                        pszMsgId, pMI);
        free ((char*)pszMsgId);
        if (pMI != NULL) {
            pRet->append (pMI);
            ui16Count++;
        }
    } while (pPrepStmt->next (pRow) && ui16Count < ui16Limit);

    delete pRow;
    delete pPrepStmt;
    _m.unlock (207);
    return pRet;
}

PtrLList<MessageId> * SQLMessageHeaderStorage::getNotReplicatedMsgList (const char *pszTargetPeer, unsigned int uiLimit,
                                                                        StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pFilters)
{
    const char *pszMethodName = "SQLMessageStorage::getNotReplicatedMsgList";

    _m.lock (209);

    if (_pGetNotSentMsgInfoPrepStmt == NULL) {
        // Create the prepared statement if it does not already exist
        String sql = (String) "SELECT " + METAINFO_FIELDS + " FROM " + TABLE_NAME +
                              " WHERE " +
                              TABLE_NAME + "." + FIELD_GROUP_NAME + " || '" + MessageHeader::MSG_ID_SEPARATOR + "' || " +
                              TABLE_NAME + "." + FIELD_SENDER_ID + " || '" + MessageHeader::MSG_ID_SEPARATOR + "' || " +
                              TABLE_NAME + "." + FIELD_MSG_SEQ_ID + " || '" + MessageHeader::MSG_ID_SEPARATOR + "' || " +
                              TABLE_NAME + "." + FIELD_CHUNK_ID + " || '" + MessageHeader::MSG_ID_SEPARATOR + "' || " +
                              TABLE_NAME + "." + FIELD_FRAGMENT_OFFSET + " || '" + MessageHeader::MSG_ID_SEPARATOR + "' || " +
                              TABLE_NAME + "." + FIELD_FRAGMENT_LENGTH  +
                              " NOT IN (" +
                                  "SELECT " + SQLTransmissionHistory::TABLE_MESSAGE_IDS  + "." + SQLTransmissionHistory::MSG_ID +
                                  " FROM " + SQLTransmissionHistory::TABLE_MESSAGE_IDS + ", " + SQLTransmissionHistory::TABLE_REL + ", " + SQLTransmissionHistory::TABLE_TARGETS +
                                  " WHERE " + SQLTransmissionHistory::TABLE_MESSAGE_IDS + "." + SQLTransmissionHistory::MSG_ROW_ID + " = " +
                                  SQLTransmissionHistory::TABLE_REL + "." + SQLTransmissionHistory::MSG_ROW_ID +
                                  " AND " +
                                  SQLTransmissionHistory::TABLE_TARGETS + "." + SQLTransmissionHistory::TARGET_ROW_ID + " = " +
                                  SQLTransmissionHistory::TABLE_REL + "." + SQLTransmissionHistory::TARGET_ROW_ID +
                                  " AND " +
                                  SQLTransmissionHistory::TABLE_TARGETS + "." + SQLTransmissionHistory::TARGET_ID + " = ?1)" +
                              " AND "  + FIELD_FRAGMENT_LENGTH + " = " + FIELD_TOT_MSG_LENGTH +
                              " ORDER BY "  + FIELD_CHUNK_ID + " ASC;";

        _pGetNotSentMsgInfoPrepStmt = _pDB->prepare (sql.c_str());
        if (_pGetNotSentMsgInfoPrepStmt != NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "statement %s prepared successfully\n", (const char *)sql);
        }
        else {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMsgInfo", Logger::L_SevereError,
                            "could not prepare statement: %s\n", (const char *)sql);
            _m.unlock (209);
            return NULL;
        }
    }

    // Bind the values to the prepared statement 
    if (_pGetNotSentMsgInfoPrepStmt->bind (1, pszTargetPeer)) { 
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, 
                        "error when binding values\n"); 
        _pGetNotSentMsgInfoPrepStmt->reset();
        _m.unlock (209);
        return NULL; 
    } 

    // Execute the statement 
    int iElements = 0;
    PtrLList<MessageId> *pList = getMessageIds (_pGetNotSentMsgInfoPrepStmt, &iElements, true);
    checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                    "database select identified %lu messages that were not replicated to target %s (prior to filtering based on receiver list)\n",
                    pList->getCount(), pszTargetPeer);
    _pGetNotSentMsgInfoPrepStmt->reset(); 

    if (pList != NULL) {
        MessageId *pMId;
        pList->resetGet();
        for (unsigned int uiCounter = 0; (pMId = pList->getNext()) != NULL;) { 
            if (uiLimit == 0 || uiCounter < uiLimit) {
                bool bRemoved = false;
                if (pFilters != NULL) {
                    ReceivedMessages::ReceivedMsgsByGrp *pRcvdMsgByGrp = pFilters->get (pMId->getGroupName());
                    if (pRcvdMsgByGrp != NULL) {
                        ReceivedMessages::ReceivedMsgsByPub *pRcvdMsgByPub = pRcvdMsgByGrp->msgsByPub.get (pMId->getOriginatorNodeId());
                        if (pRcvdMsgByPub != NULL) {
                            if (pRcvdMsgByPub->ranges.hasTSN (pMId->getSeqId())) {
                                checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug,
                                                "filtered out message %s since it was already in the destination's database\n",
                                                pMId->getId());
                                delete pList->remove (pMId);
                                bRemoved = true;
                            }
                        }
                    }
                }
                if (!bRemoved) {
                    uiCounter++;
                }
            }
            else {
                delete pList->remove (pMId);
            }
        }

        if (pList->getFirst() == NULL) {
            delete pList;
            pList = NULL;
        }
    }
    _m.unlock (209); 
    return pList; 
}

int SQLMessageHeaderStorage::countChunks (const char *pszGroupName, const char *pszPublisherNodeId,
                                          uint32 ui32MsgSeqId, unsigned int &uiTotNChunks)
{
    uiTotNChunks = 0;
    if (pszGroupName == NULL || pszPublisherNodeId == NULL) {
        return -1;
    }

    _m.lock (208);
    const char *pszMethodName = "SQLMessageStorage::countChunks";
    if (_pCountChunksPrepStmt == NULL) {
        // Create the prepared statement if it does not already exist
        String sql = (String) "SELECT COUNT(*), "
                              "MAX (" + FIELD_TOT_N_CHUNKS + "), "
                              "MIN (" + FIELD_CHUNK_ID + "), "
                              "MAX (" + FIELD_CHUNK_ID + ") "
                              "FROM " + TABLE_NAME + " WHERE "
                             + FIELD_GROUP_NAME + " = ?1 AND "
                             + FIELD_SENDER_ID + " = ?2 AND "
                             + FIELD_MSG_SEQ_ID + " = ?3 AND "
                             //+ FIELD_CHUNK_ID + " <> " + MessageHeader::UNDEFINED_CHUNK_ID + " AND"
                             + FIELD_FRAGMENT_LENGTH + " = "  // Select only _complete_
                             + FIELD_TOT_MSG_LENGTH + ";";    // chunks

        _pCountChunksPrepStmt = _pDB->prepare (sql.c_str());
        if (_pCountChunksPrepStmt != NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "Statement %s prepared successfully.\n", (const char *)sql);
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "Could not prepare statement: %s.\n", (const char *)sql);
            _m.unlock (208);
            return -2;
        }
    }

    // Bind the values to the prepared statement
    if (_pCountChunksPrepStmt->bind (1, pszGroupName) < 0 ||
        _pCountChunksPrepStmt->bind (2, pszPublisherNodeId) < 0 ||
        _pCountChunksPrepStmt->bind (3, ui32MsgSeqId)) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Error when binding values.\n");
        _pCountChunksPrepStmt->reset();
        _m.unlock (208);
        return -3;
    }

    Row *pRow = _pCountChunksPrepStmt->getRow();
    if (pRow == NULL) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        _m.unlock (208);
        return -4;
    }

    int iCount, nChunks;
    iCount = nChunks = 0;
    
    uint8 uint8MinChunkId, ui8MaxChunkId;
    uint8MinChunkId = MessageHeader::MIN_CHUNK_ID;
    ui8MaxChunkId = MessageHeader::MAX_CHUNK_ID;
    if (_pCountChunksPrepStmt->next (pRow)) {
        pRow->getValue (0, iCount);
        pRow->getValue (1, nChunks);
        pRow->getValue (2, uint8MinChunkId);
        pRow->getValue (3, ui8MaxChunkId);
        if (_pCountChunksPrepStmt->next (pRow)) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "unexpected number of records in the database for message id <%s:%s:%lu>\n",
                            pszGroupName, pszPublisherNodeId, ui32MsgSeqId);
            _pCountChunksPrepStmt->reset();
            delete pRow;
            _m.unlock (208);
            return -5;
        }

        if ((iCount > 0) && ((uint8MinChunkId == MessageHeader::UNDEFINED_CHUNK_ID ||
            ui8MaxChunkId == MessageHeader::UNDEFINED_CHUNK_ID))) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "method was called "
                            "for a pushed message (which was not chunked)\n");
            _pCountChunksPrepStmt->reset();
            delete pRow;
            _m.unlock (208);
            return -6;
        }
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "unexpected return from query\n");
        _pCountChunksPrepStmt->reset();
        delete pRow;
        _m.unlock (208);
        return -7;
    }

    _pCountChunksPrepStmt->reset();
    delete pRow;
    _m.unlock (208);
    if (iCount > nChunks) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "count of the number of chunks (%d) exceeds the total number of chunks (%d)\n",
                        iCount, nChunks);
        return -8;
    }
    if (nChunks < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "invalid total number of chunks (%d)\n",
                        nChunks);
        return -9;
    }
    if (nChunks >= 0) {
        uiTotNChunks = (unsigned int) nChunks;
    }

    return iCount;
}

/*
MessageHeader * SQLMessageHeaderStorage::getMsgInfo (MessageHeader *pMH)
{
    if (pMH == NULL) {
        return NULL;
    }

    return getMsgInfo (pMH->getGroupName(), pMH->getPublisherNodeId(),
                       pMH->getMsgSeqId(), pMH->getChunkId(),
                       pMH->getFragmentOffset(), pMH->getFragmentLength());
}*/

MessageHeader * SQLMessageHeaderStorage::getMsgInfo (const char *pszKey)
{
    if (pszKey == NULL) {
        return NULL;
    }

    DArray2<String> tokens (6);
    if (isFullyQualifiedMessageID (pszKey)) {
        if (convertKeyToField (pszKey, tokens) != 0) {
            return NULL;
        }
        return getMsgInfo (tokens[MSG_ID_GROUP], tokens[MSG_ID_SENDER],
                           atoui32 (tokens[MSG_ID_SEQ_NUM]), (uint8) atoi (tokens[MSG_ID_CHUNK_ID]),
                           atoui32 (tokens[MSG_ID_OFFSET]), atoui32 (tokens[MSG_ID_LENGTH]));
    }
    else {
        String msgId (pszKey);
        if (isAllChunksMessageID (msgId) && !isOnDemandDataID (msgId)) {
            msgId += ":0";
        }
        if (convertKeyToField (msgId, tokens, 4, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM, MSG_ID_CHUNK_ID) != 0) {
            return NULL;
        }
        return getMsgInfo (tokens[MSG_ID_GROUP], tokens[MSG_ID_SENDER],
                           atoui32 (tokens[MSG_ID_SEQ_NUM]), (uint8) atoi (tokens[MSG_ID_CHUNK_ID]));
    }
}

MessageHeader * SQLMessageHeaderStorage::getMsgInfo (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId,
                                                     uint8 ui8Chunk, uint32 ui32FragOffset, uint32 ui32FragLength)
{
    const char *pszMethodName = "SQLMessageStorage::getMsgInfo";

    IHMC_MISC::PreparedStatement *pQueryStmt;
    if (ui32FragLength != 0) {
        // Looking for a specific fragment
        if (_pGetMsgInfoPrepStmt == NULL) {
            // Create the prepared statement if it does not already exist
            String sql = (String) "SELECT " + METAINFO_FIELDS + " FROM " + TABLE_NAME + " WHERE "
                                            + FIELD_GROUP_NAME + " = ?1 AND "
                                            + FIELD_SENDER_ID + " = ?2 AND "
                                            + FIELD_MSG_SEQ_ID + " = ?3 AND "
                                            + FIELD_CHUNK_ID + " = ?4 AND "
                                            + FIELD_FRAGMENT_OFFSET +" = ?5 AND "
                                            + FIELD_FRAGMENT_LENGTH + " = ?6;";
            _pGetMsgInfoPrepStmt = _pDB->prepare (sql.c_str());
            if (_pGetMsgInfoPrepStmt != NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_Info,
                                "statement %s prepared successfully\n", (const char *) sql);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "failed to prepare statement: %s\n", (const char *) sql);
                return NULL;
            }
        }
        // Bind the values to the prepared statement
        if (_pGetMsgInfoPrepStmt->bind (1, pszGroupName) < 0 ||
            _pGetMsgInfoPrepStmt->bind (2, pszSenderNodeId) < 0 ||
            _pGetMsgInfoPrepStmt->bind (3, ui32MsgSeqId) < 0 ||
            _pGetMsgInfoPrepStmt->bind (4, ui8Chunk) < 0 ||
            _pGetMsgInfoPrepStmt->bind (5, ui32FragOffset) < 0 ||
            _pGetMsgInfoPrepStmt->bind (6, ui32FragLength) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "error when binding values.\n");
            _pGetMsgInfoPrepStmt->reset();
            return NULL;
        }
        pQueryStmt = _pGetMsgInfoPrepStmt;
    }
    else {
        // Looking for a complete message
        if (_pGetCompleteMsgInfoPrepStmt == NULL) {
            // Create the prepared statement if it does not already exist
            String sql = (String) "SELECT " + METAINFO_FIELDS + " FROM " + TABLE_NAME + " WHERE "
                                            + FIELD_GROUP_NAME + " = ?1 AND "
                                            + FIELD_SENDER_ID + " = ?2 AND "
                                            + FIELD_MSG_SEQ_ID + " = ?3 AND "
                                            + FIELD_CHUNK_ID + " = ?4 AND "
                                            + FIELD_FRAGMENT_OFFSET +" = 0 AND "
                                            + FIELD_FRAGMENT_LENGTH + " = " + FIELD_TOT_MSG_LENGTH + ";";
            _pGetCompleteMsgInfoPrepStmt = _pDB->prepare (sql.c_str());
            if (_pGetCompleteMsgInfoPrepStmt != NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_Info,
                                "statement %s prepared successfully\n", (const char *) sql);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "failed to prepare statement: %s\n", (const char *) sql);
                return NULL;
            }
        }
        // Bind the values to the prepared statement
        if (_pGetCompleteMsgInfoPrepStmt->bind (1, pszGroupName) < 0 ||
            _pGetCompleteMsgInfoPrepStmt->bind (2, pszSenderNodeId) < 0 ||
            _pGetCompleteMsgInfoPrepStmt->bind (3, ui32MsgSeqId) < 0 ||
            _pGetCompleteMsgInfoPrepStmt->bind (4, ui8Chunk) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "error when binding values.\n");
            _pGetCompleteMsgInfoPrepStmt->reset();
            return NULL;
        }
        pQueryStmt = _pGetCompleteMsgInfoPrepStmt;
    }
    
    // Execute the statement
    _m.lock (209);
    int iElements = 0;
    PtrLList<MessageHeader> *pList = getMessageInfo (pQueryStmt, iElements);
    pQueryStmt->reset();

    if (iElements == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                        "did not find any match for %s:%s:%u:%u:%u:%u\n",
                        pszGroupName, pszSenderNodeId, ui32MsgSeqId,
                        ui8Chunk, ui32FragOffset, ui32FragLength);
    }
    else if (iElements > 1) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "found %d matches for key <%s:%s:%u:%u:%u:%u>, but there should have only been one match\n",
                        iElements, pszGroupName, pszSenderNodeId, ui32MsgSeqId,
                        ui8Chunk, ui32FragOffset, ui32FragLength);
    }

    MessageHeader *pMH = NULL;
    if (pList != NULL) {
        pMH = pList->getFirst();
        // Delete all the element in the list, besides the first one (that is
        // returned).
        // NB: the query should always return one and only one element, thus the
        //     following loop should never be executed
        MessageHeader *pTmp;
        while ((pTmp = pList->getNext()) != NULL) {
            delete pTmp;
            pTmp = NULL;
        }
        delete pList;
        pList = NULL;
    }
    _m.unlock (209);
    return pMH;
}

PtrLList<MessageHeader> * SQLMessageHeaderStorage::getMatchingFragments (const char *pszGroupName, const char *pszSender,
                                                                         uint32 ui32MsgSeqId)
{
    _m.lock (210);
    if (_pGetMatchingFragmentMsgInfoPrepStmt_1 == NULL) {
        String sql = (String) "SELECT " + METAINFO_FIELDS + " FROM " + TABLE_NAME + " WHERE "
                             + FIELD_GROUP_NAME + " = ?1 AND "
                             + FIELD_SENDER_ID + " = ?2 AND "
                             + FIELD_MSG_SEQ_ID + " = ?3;";

        _pGetMatchingFragmentMsgInfoPrepStmt_1 = _pDB->prepare ((const char *) sql);
        if (_pGetMatchingFragmentMsgInfoPrepStmt_1 != NULL) {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", Logger::L_Info,
                            "Statement %s prepared successfully.\n", (const char *)sql);
        }
        else {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", Logger::L_SevereError,
                            "Could not prepare statement: %s.\n",
                            (const char *)sql); 
            _m.unlock (210);
            return NULL;
        }
    }

    // Bind the values to the prepared statement
    if (_pGetMatchingFragmentMsgInfoPrepStmt_1->bind (1, pszGroupName) < 0 ||
        _pGetMatchingFragmentMsgInfoPrepStmt_1->bind (2, pszSender) < 0 || 
        _pGetMatchingFragmentMsgInfoPrepStmt_1->bind (3, ui32MsgSeqId) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", bindingError);
        _pGetMatchingFragmentMsgInfoPrepStmt_1->reset();
        _m.unlock (210);
        return NULL;
    }

    // Execute the statement
    int iElements;
    PtrLList<MessageHeader> *pList = getMessageInfo (_pGetMatchingFragmentMsgInfoPrepStmt_1, iElements);
    _pGetMatchingFragmentMsgInfoPrepStmt_1->reset();
    if (iElements == 0 || pList == NULL || pList->getFirst() == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", Logger::L_HighDetailDebug,
                        "did not find any fragment matching %s:%s:%u\n",
                        pszGroupName, pszSender, ui32MsgSeqId);
    }

    _m.unlock (210);
    return pList;
}

PtrLList<MessageHeader> * SQLMessageHeaderStorage::getMatchingFragments (const char *pszGroupName, const char *pszSender,
                                                                         uint32 ui32MsgSeqId, uint32 ui32ChunkSeqId)
{
    _m.lock (211);
    if (_pGetMatchingFragmentMsgInfoPrepStmt_2 == NULL) {
        String sql = (String) "SELECT " + METAINFO_FIELDS + " FROM " + TABLE_NAME + " WHERE "
                                 + FIELD_GROUP_NAME + " = ?1 AND "
                                 + FIELD_SENDER_ID + " = ?2 AND "
                                 + FIELD_MSG_SEQ_ID + " = ?3 AND "
                                 + FIELD_CHUNK_ID + " = ?4;";

        _pGetMatchingFragmentMsgInfoPrepStmt_2 = _pDB->prepare ((const char *) sql);
        if (_pGetMatchingFragmentMsgInfoPrepStmt_2 != NULL) {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", Logger::L_Info,
                            "Statement %s prepared successfully.\n", (const char *)sql);
        }
        else {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", Logger::L_SevereError,
                            "Could not prepare statement: %s.\n",
                            (const char *)sql); 
            _m.unlock (211);
            return NULL;
        }
    }

    // Bind the values to the prepared statement
    if (_pGetMatchingFragmentMsgInfoPrepStmt_2->bind (1, pszGroupName) < 0 ||
        _pGetMatchingFragmentMsgInfoPrepStmt_2->bind (2, pszSender) < 0 || 
        _pGetMatchingFragmentMsgInfoPrepStmt_2->bind (3, ui32MsgSeqId) < 0 ||
        _pGetMatchingFragmentMsgInfoPrepStmt_2->bind (4, ui32ChunkSeqId) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", bindingError);
        _pGetMatchingFragmentMsgInfoPrepStmt_2->reset();
        _m.unlock (211);
        return NULL;
    }

    // Execute the statement
    int iElements;
    PtrLList<MessageHeader> *pList = getMessageInfo (_pGetMatchingFragmentMsgInfoPrepStmt_2, iElements);
    _pGetMatchingFragmentMsgInfoPrepStmt_2->reset();
    if (iElements == 0 || pList == NULL || pList->getFirst() == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", Logger::L_HighDetailDebug,
                        "did not find any fragment matching %s:%s:%u:%u\n",
                        pszGroupName, pszSender, ui32MsgSeqId);
    }

    _m.unlock (211);
    return pList;
}

PtrLList<MessageHeader> * SQLMessageHeaderStorage::getMatchingFragments (const char *pszGroupName, const char *pszSender,
                                                                         uint32 ui32MsgSeqId, uint32 ui32ChunkSeqId,
                                                                         uint32 ui32StartOffset, uint32 ui32EndOffset)
{
    _m.lock (212);
    if (_pGetMatchingFragmentMsgInfoPrepStmt_3 == NULL) {
        String rangeStatement = (String)
                            "("
                          +      FIELD_FRAGMENT_OFFSET + " >= ?5 AND " + FIELD_FRAGMENT_OFFSET + " < ?6" +
                          + ")"
                          + " OR "
                          + "("
                          +     "(" + FIELD_FRAGMENT_OFFSET + " + " + FIELD_FRAGMENT_LENGTH + ") > ?5"
                          +     " AND "
                          +     "(" + FIELD_FRAGMENT_OFFSET + " + " + FIELD_FRAGMENT_LENGTH + ") <= ?6"
                          + ")"
                          + " OR "
                          + "("
                          +     FIELD_FRAGMENT_OFFSET + " <= ?5"
                          +     " AND "
                          +     "(" + FIELD_FRAGMENT_OFFSET + " + " + FIELD_FRAGMENT_LENGTH + ") >= ?6"
                          + ")";

        String sql = (String) "SELECT " + METAINFO_FIELDS + " FROM " + TABLE_NAME + " WHERE "
                                 + FIELD_GROUP_NAME + " = ?1 AND "
                                 + FIELD_SENDER_ID + " = ?2 AND "
                                 + FIELD_MSG_SEQ_ID + " = ?3 AND "
                                 + FIELD_CHUNK_ID + " = ?4 AND "
                                 + "(" + rangeStatement + ");";

        _pGetMatchingFragmentMsgInfoPrepStmt_3 = _pDB->prepare ((const char *) sql);
        if (_pGetMatchingFragmentMsgInfoPrepStmt_3 != NULL) {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", Logger::L_Info,
                            "Statement %s prepared successfully.\n", (const char *)sql);
        }
        else {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", Logger::L_SevereError,
                            "Could not prepare statement: %s.\n",
                            (const char *)sql); 
            _m.unlock (212);
            return NULL;
        }
    }

    // Bind the values to the prepared statement
    if (_pGetMatchingFragmentMsgInfoPrepStmt_3->bind (1, pszGroupName) < 0 ||
        _pGetMatchingFragmentMsgInfoPrepStmt_3->bind (2, pszSender) < 0 || 
        _pGetMatchingFragmentMsgInfoPrepStmt_3->bind (3, ui32MsgSeqId) < 0 ||
        _pGetMatchingFragmentMsgInfoPrepStmt_3->bind (4, ui32ChunkSeqId) < 0 ||
        _pGetMatchingFragmentMsgInfoPrepStmt_3->bind (5, ui32StartOffset) < 0 ||
        _pGetMatchingFragmentMsgInfoPrepStmt_3->bind (6, ui32EndOffset)) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", bindingError);
        _pGetMatchingFragmentMsgInfoPrepStmt_3->reset();
        _m.unlock (212);
        return NULL;
    }

    // Execute the statement
    int iElements;

    PtrLList<MessageHeader> *pList = getMessageInfo (_pGetMatchingFragmentMsgInfoPrepStmt_3, iElements);
    _pGetMatchingFragmentMsgInfoPrepStmt_3->reset();

    if (iElements == 0 || pList == NULL || pList->getFirst() == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMatchingFragments", Logger::L_HighDetailDebug,
                        "did not find any fragment matching %s:%s:%u:%u:%u:%u\n",
                        pszGroupName, pszSender, ui32MsgSeqId, ui32ChunkSeqId,
                        ui32StartOffset, ui32EndOffset);
    }

    _m.unlock (212);
    return pList;
}

PtrLList<MessageHeader> * SQLMessageHeaderStorage::getCompleteChunkMessageInfos (const char *pszGroupName, const char *pszSender,
                                                                                 uint32 ui32MsgSeqId)
{
    _m.lock (213);
    if (_pGetComplChunkMsgHeadersPrepStmt == NULL) {
        String sql = (String) "SELECT " + METAINFO_FIELDS + " FROM " + TABLE_NAME + " WHERE "
                                 + FIELD_GROUP_NAME + " = ?1 AND "
                                 + FIELD_SENDER_ID + " = ?2 AND "
                                 + FIELD_MSG_SEQ_ID + " = ?3 AND "
                                 + FIELD_FRAGMENT_LENGTH + " = " + FIELD_TOT_MSG_LENGTH + ";";

        _pGetComplChunkMsgHeadersPrepStmt = _pDB->prepare ((const char *) sql);
        if (_pGetComplChunkMsgHeadersPrepStmt != NULL) {
            checkAndLogMsg ("SQLMessageHeaderStorage::getCompleteChunkMessageInfos", Logger::L_Info,
                            "Statement %s prepared successfully.\n", (const char *)sql);
        }
        else {
            checkAndLogMsg ("SQLMessageHeaderStorage::getCompleteChunkMessageInfos", Logger::L_SevereError,
                            "Could not prepare statement: %s.\n",
                            (const char *)sql); 
            _m.unlock (213);
            return NULL;
        }
    }

    // Bind the values to the prepared statement
    if (_pGetComplChunkMsgHeadersPrepStmt->bind (1, pszGroupName) < 0 ||
        _pGetComplChunkMsgHeadersPrepStmt->bind (2, pszSender) < 0 || 
        _pGetComplChunkMsgHeadersPrepStmt->bind (3, ui32MsgSeqId) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getCompleteChunkMessageInfos", bindingError);
        _pGetComplChunkMsgHeadersPrepStmt->reset();
        _m.unlock (213);
        return NULL;
    }

    // Execute the statement
    int iElements;
    PtrLList<MessageHeader> *pList = getMessageInfo (_pGetComplChunkMsgHeadersPrepStmt, iElements);
    _pGetComplChunkMsgHeadersPrepStmt->reset();
    if (iElements == 0 || pList == NULL || pList->getFirst() == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getCompleteChunkMessageInfos", Logger::L_HighDetailDebug,
                        "did not find any fragment matching %s:%s:%u\n", pszGroupName, pszSender,
                        ui32MsgSeqId);
        if (pList != NULL) {
            delete pList;
            pList = NULL;
        }
    }

    _m.unlock (213);
    return pList;
}

PtrLList<MessageHeader> * SQLMessageHeaderStorage::getMessageInfo (PreparedStatement *pStmt, int &iElements, bool bMaintainOrder)
{
    iElements = 0;
    PtrLList<MessageHeader> *pList = NULL;

    Row *pRow = pStmt->getRow();
    if (pRow == NULL) {
        return NULL;
    }

    while (pStmt->next (pRow)) {
        MessageHeader *pMI = getMessageInfo (pRow);
        if (pMI == NULL) {
            break;
        }
        if (pList == NULL) {
            pList = new PtrLList<MessageHeader>();
            if (pList == NULL) {
                checkAndLogMsg ("SQLMessageHeaderStorage::getMsgInfo", memoryExhausted);
                break;
            }
        }

        if (bMaintainOrder) {
            pList->append (pMI);
        }
        else {
            pList->prepend (pMI);
        }
        iElements++;
    }

    delete pRow;
    pStmt->reset();

    return pList;
}

MessageHeader * SQLMessageHeaderStorage::getMessageInfo (Row *pRow)
{
    if (pRow == NULL) {
        return NULL;
    }

    char *pszGroupName = NULL;
    char *pszSenderNodeId = NULL;
    char *pszObjectId = NULL;
    char *pszInstanceId = NULL;
    char *pszMimeType = NULL;
    char *pszChecksum = NULL;
    uint8 ui8ChunkId, ui8ClientType, ui8Priority, ui8Acknoledgment, ui8MetaData, ui8TotalNumOfChunks;
    uint16 ui16Tag, ui16ClientId, ui16HistoryWindow;
    uint32 ui32MsgSeqId, ui32TotalMessageLength, ui32FragmentLength,
    ui32FragmentOffset, ui32MetaDataLength;
    uint64 ui64Expiration;

    pRow->getValue (FIELD_GROUP_NAME_COLUMN_NUMBER, &pszGroupName);
    pRow->getValue (FIELD_SENDER_ID_COLUMN_NUMBER, &pszSenderNodeId);
    pRow->getValue (FIELD_MSG_SEQ_ID_COLUMN_NUMBER, ui32MsgSeqId);
    pRow->getValue (FIELD_CHUNK_ID_COLUMN_NUMBER, ui8ChunkId);
    pRow->getValue (FIELD_OBJECT_ID_COLUMN_NUMBER, &pszObjectId);
    pRow->getValue (FIELD_INSTANCE_ID_COLUMN_NUMBER, &pszInstanceId);
    pRow->getValue (FIELD_TAG_COLUMN_NUMBER, ui16Tag);
    pRow->getValue (FIELD_CLIENT_ID_COLUMN_NUMBER, ui16ClientId);
    pRow->getValue (FIELD_CLIENT_TYPE_COLUMN_NUMBER, ui8ClientType);
    pRow->getValue (FIELD_MIME_TYPE_COLUMN_NUMBER, &pszMimeType);
    pRow->getValue (FIELD_CHECKSUM_COLUMN_NUMBER, &pszChecksum);
    pRow->getValue (FIELD_TOT_MSG_LENGTH_COLUMN_NUMBER, ui32TotalMessageLength);
    pRow->getValue (FIELD_FRAGMENT_LENGTH_COLUMN_NUMBER, ui32FragmentLength);
    pRow->getValue (FIELD_FRAGMENT_OFFSET_COLUMN_NUMBER, ui32FragmentOffset);
    pRow->getValue (FIELD_METADATA_LENGTH_COLUMN_NUMBER, ui32MetaDataLength);
    pRow->getValue (FIELD_TOT_N_CHUNKS_NUMBER, ui8TotalNumOfChunks);
    pRow->getValue (FIELD_HISTORY_WINDOW_COLUMN_NUMBER, ui16HistoryWindow);
    pRow->getValue (FIELD_PRIORITY_COLUMN_NUMBER, ui8Priority);
    pRow->getValue (FIELD_EXPIRATION_COLUMN_NUMBER, ui64Expiration);
    pRow->getValue (FIELD_ACKNOLEDGMENT_COLUMN_NUMBER, ui8Acknoledgment);
    pRow->getValue (FIELD_METADATA_COLUMN_NUMBER, ui8MetaData);

    // Sanity check
    if ((ui64Expiration < 2247483648U) && (ui64Expiration != 0)) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMessageInfo", Logger::L_SevereError,
                        "64 bit conversion error (the value retrieved is %lu)\n", ui64Expiration);
    }
    if ((ui8Acknoledgment != 1) && (ui8Acknoledgment != 0)) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMsgInfo", Logger::L_SevereError,
                        "sqlite3_get_table() returned a non-bool type for ui8Acknoledgment\n");
        free (pszGroupName);
        free (pszSenderNodeId);
        return NULL;
    }
    if ((ui8MetaData != 1) && (ui8MetaData != 0)) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMsgInfo", Logger::L_SevereError,
                        "sqlite3_get_table() returned a non-bool type for ui8Acknoledgment\n");
        free (pszGroupName);
        free (pszSenderNodeId);
        return NULL;
    }

    MessageHeader *pMH = MessageHeaderHelper::getMessageHeader (pszGroupName, pszSenderNodeId, ui32MsgSeqId, ui8ChunkId,
                                                                pszObjectId, pszInstanceId, ui16Tag, ui16ClientId, ui8ClientType,
                                                                pszMimeType, pszChecksum, ui32TotalMessageLength,
                                                                ui32FragmentOffset, ui32FragmentLength, ui32MetaDataLength,
                                                                ui16HistoryWindow, ui8Priority, ui64Expiration,
                                                                ui8Acknoledgment != 0 ? true : false, ui8MetaData != 0 ? true: false,
                                                                ui8TotalNumOfChunks);

    // MessageHeader's constructor makes a copy of pszGroupName and pszSenderNodeId,
    // they can therefore be deallocated
    if (pszGroupName != NULL) {
        free (pszGroupName);
        pszGroupName = NULL;
    }
    if (pszSenderNodeId != NULL) {
        free (pszSenderNodeId);
        pszSenderNodeId = NULL;
    }
    if (pszObjectId != NULL) {
        free (pszObjectId);
        pszObjectId = NULL;
    }
    if (pszInstanceId !=  NULL) {
        free (pszInstanceId);
        pszInstanceId = NULL;
    }
    if (pszMimeType != NULL) {
        free (pszMimeType);
        pszMimeType = NULL;
    }
    if (pszChecksum != NULL) {
        free (pszChecksum);
        pszChecksum = NULL;
    }
    if (pMH == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMsgInfo", memoryExhausted);
    }

    return pMH;
}

PtrLList<MessageId> * SQLMessageHeaderStorage::getMessageIds (PreparedStatement *pStmt, int *piResultCount, bool bMantainOrder)
{
    int iCount = 0;
    PtrLList<MessageId> *pList = NULL;

    if (piResultCount != NULL) {
        *piResultCount = 0;
    }

    Row *pRow = pStmt->getRow();
    if (pRow == NULL) {
        return NULL;
    }

    while (pStmt->next (pRow)) {
        MessageId *pMId = getMessageId (pRow);
        if (pMId == NULL) {
            break;
        }
        if (pList == NULL) {
            pList = new PtrLList<MessageId>();
            if (pList == NULL) {
                checkAndLogMsg ("SQLMessageHeaderStorage::getMsgInfo", memoryExhausted);
                break;
            }
        }

        if (bMantainOrder) {
            pList->append (pMId);
        }
        else {
            pList->prepend (pMId);
        }
        iCount++;
    }

    delete pRow;
    pStmt->reset();

    return pList;
}

MessageId * SQLMessageHeaderStorage::getMessageId (Row *pRow)
{
    if (pRow == NULL) {
        return NULL;
    }

    char *pszGroupName = NULL;
    char *pszSenderNodeId = NULL;
    uint32 ui32MsgSeqId;
    uint8 ui8ChunkId;

    pRow->getValue (FIELD_GROUP_NAME_COLUMN_NUMBER, &pszGroupName);
    pRow->getValue (FIELD_SENDER_ID_COLUMN_NUMBER, &pszSenderNodeId);
    pRow->getValue (FIELD_MSG_SEQ_ID_COLUMN_NUMBER, ui32MsgSeqId);
    pRow->getValue (FIELD_CHUNK_ID_COLUMN_NUMBER, ui8ChunkId);
    
    MessageId *pMId = new MessageId (pszGroupName, pszSenderNodeId, ui32MsgSeqId, ui8ChunkId);
    
    // MessageId's constructor makes a copy of pszGroupName and pszSenderNodeId,
    // they can therefore be deallocated
    free (pszGroupName);
    pszGroupName = NULL;
    free (pszSenderNodeId);
    pszSenderNodeId = NULL;

    return pMId;
}

const char * SQLMessageHeaderStorage::getId (Row *pRow)
{
    unsigned int uiNCols= pRow->getColumnCount();
    if (uiNCols < MSG_ID_N_FIELDS) {
        return NULL;
    }

    String id;
    // get the fields that are part of the key and build it
    for (unsigned int i = 0; i < uiNCols; i++) {
        char *pszVal = NULL;
        if (pRow->getValue (i, &pszVal) < 0) {
            if (pszVal != NULL) {
                free (pszVal);
                return NULL;
            }
        }
        if (i > 0) {
            id += ":";
        }
        id += pszVal;  // String makes a copy of pszVal
    }

    return id.r_str();
}

uint32 SQLMessageHeaderStorage::getNextExpectedSeqId (const char *pszGroupName, const char *pszPublisherNodeId, uint16 ui6Tag)
{
    char buf[12];
    String sql = "SELECT MAX ( ";
    sql = sql + FIELD_MSG_SEQ_ID + ") FROM ";
    sql = sql + TABLE_NAME + " WHERE " + FIELD_GROUP_NAME + " = '" + pszGroupName + "' AND ";
    sql = sql + FIELD_SENDER_ID + " ='" + pszPublisherNodeId + "'";
    if (ui6Tag != 0) {
        sql = sql + " AND " + FIELD_TAG + " = " + itoa(buf, ui6Tag) + ";";
    }

    _m.lock (215);

    PreparedStatement *pStmt = _pDB->prepare ((const char *)sql);
    if (pStmt == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getNextExpectedSeqId", Logger::L_SevereError,
                        "could not prepare statement %s\n", (const char *) sql);
    }

    Row *pRow = pStmt->getRow();
    if (pRow == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getNextExpectedSeqId", memoryExhausted);
        _m.unlock (215);
        return 0;
    }

    uint32 ui32ExpectedSeqId = 0;
    if (pStmt->next (pRow)) {
        uint32 ui32LastSeqId = 0;
        pRow->getValue (0, ui32LastSeqId);
        ui32ExpectedSeqId = ui32LastSeqId + 1;
    }
    else {
        checkAndLogMsg ("SQLMessageHeaderStorage::getNextExpectedSeqId", Logger::L_Info,
                        "query %s did not return any result\n", (const char *) sql);
    }
    delete pRow;
    delete pStmt;

    _m.unlock (215);
    return ui32ExpectedSeqId;
}

char * SQLMessageHeaderStorage::getCreateTableSQLStatement()
{
    String sql = (String) "CREATE TABLE IF NOT EXISTS " + TABLE_NAME + " (";

    sql = sql + FIELD_GROUP_NAME + " TEXT, ";
    sql = sql + FIELD_SENDER_ID + " TEXT, ";
    sql = sql + FIELD_MSG_SEQ_ID + " INT, ";

    sql = sql + FIELD_CHUNK_ID + " INT, ";

    sql = sql + FIELD_OBJECT_ID + " TEXT, ";
    sql = sql + FIELD_INSTANCE_ID + " TEXT, ";

    sql = sql + FIELD_TAG + " INT, ";

    sql = sql + FIELD_CLIENT_ID + " INT, ";
    sql = sql + FIELD_CLIENT_TYPE + " INT, ";

    sql = sql + FIELD_MIME_TYPE + " TEXT, ";
    sql = sql + FIELD_CHECKSUM + " TEXT, ";

    sql = sql + FIELD_TOT_MSG_LENGTH +" INT, ";
    sql = sql + FIELD_FRAGMENT_LENGTH +" INT, ";
    sql = sql + FIELD_FRAGMENT_OFFSET +" INT, ";
    sql = sql + FIELD_METADATA_LENGTH +" INT, ";

    sql = sql + FIELD_TOT_N_CHUNKS + " INT, ";
    sql = sql + FIELD_HISTORY_WINDOW + " INT, ";
    sql = sql + FIELD_PRIORITY +" INT, ";

    sql = sql + FIELD_EXPIRATION +" INT, ";
    sql = sql + FIELD_ACKNOLEDGMENT +" INT, ";
    sql = sql + FIELD_METADATA +" INT, ";
    sql = sql + FIELD_ARRIVAL_TIMESTAMP +" INT, ";

    sql += "PRIMARY KEY (";
    sql = sql + PRIMARY_KEY + "));";

    return sql.r_str();
}

char * SQLMessageHeaderStorage::getInsertIntoTableSQLStatement()
{
    String sql = "INSERT INTO ";
    sql = sql + TABLE_NAME + " (";
    sql = sql + ALL + ")";
    sql = sql + " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    return sql.r_str();
}

PtrLList<StorageInterface::RetrievedSubscription> * SQLMessageHeaderStorage::retrieveSubscriptionGroups (const char *pszSenderNodeId)
{
    const char *pszMethodName = "SQLMessageHeaderStorage::retrieveSubscriptionGroups";

    String sql = "SELECT ";
    sql = (String) sql + FIELD_GROUP_NAME + ", " + FIELD_TAG + " FROM ";
    sql = (String) sql + TABLE_NAME + " WHERE " + FIELD_SENDER_ID + " ='" + pszSenderNodeId + "' GROUP BY ";
    sql = (String) sql + FIELD_GROUP_NAME + ", " + FIELD_TAG + ";";

    _m.lock (216);
    if (_pDB == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "_pDB is NULL\n");
        _m.unlock (216);
        return NULL;
    }

    PreparedStatement *pStmt = _pDB->prepare ((const char *)sql);
    if (pStmt == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Could not prepare statement%s\n", (const char *) sql);
        _m.unlock (216);
        return NULL;
    }
    
    Row *pRow = pStmt->getRow();
    if (pRow == NULL) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        delete pStmt;
        _m.unlock (216);
        return NULL;
    }

    if (!pStmt->next (pRow)) {
        delete pRow;
        delete pStmt;
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "Error generated by query %s\n", (const char *) sql);
        _m.unlock (216);
        return NULL;
    }

    char *pszGroupName;
    uint16 ui16Tag;
    PtrLList<RetrievedSubscription> *pRet = new PtrLList<RetrievedSubscription>();
    if (pRow == NULL) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        delete pRow;
        delete pStmt;
        _m.unlock (216);
        return NULL;
    }

    do {
        if (pRow->getValue (0, &pszGroupName) < 0 ||
            pRow->getValue (1, ui16Tag) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not "
                            "retrieve group name or tag from row matched by "
                            "query %s\n", (const char *) sql);
            continue;
        }
        RetrievedSubscription *pRSub = new RetrievedSubscription;
        if (pRSub != NULL) {
            pRSub->pszGroupName = pszGroupName;
            pRSub->ui16Tag = ui16Tag;
            pRet->prepend (pRSub);
        }
        else {
            checkAndLogMsg (pszMethodName, memoryExhausted);
            break;
        }
    } while (pStmt->next (pRow));

    if (pRet->getFirst() == NULL) {
        delete pRet;
        pRet = NULL;
    }

    delete pRow;
    delete pStmt;
    _m.unlock (216);
    return pRet;
}

//==============================================================================
//  SELECT
//==============================================================================
PtrLList<MessageHeader> * SQLMessageHeaderStorage::execSelectMsgInfo (const char *pszGroupName, const char *pszSenderNodeId, uint8 ui8ClientType,
                                                                      uint16 ui6Tag, uint32 ui32From, uint32 ui32To, uint32 ui32OffsetStart, 
                                                                      uint32 offsetEnd, uint16 ui16Limit, const char *pszOrderBy, bool bDescOrder, 
                                                                      DArray2<String> *pDAArgs)
{
    _m.lock (217);
    DArray2<String> *pKeys = execSelectID (pszGroupName, pszSenderNodeId, ui8ClientType,
                                           ui6Tag, ui32From, ui32To, ui32OffsetStart,
                                           offsetEnd, ui16Limit, pszOrderBy, bDescOrder,
                                           pDAArgs);
    _m.unlock (217);
    if (pKeys == NULL || pKeys->getSize() == 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::execSelectMsgInfo", Logger::L_Info,
                        "execSelectID returned NULL.\n");
        return NULL;
    }

    PtrLList<MessageHeader> *pRet = new PtrLList<MessageHeader>();
    MessageHeader *pMI;

    for (unsigned int i = 0; i < pKeys->size(); i++) {
        if (pKeys->used (i)) {
            pMI = getMsgInfo ((*pKeys)[i]);
            checkAndLogMsg ("SQLMessageHeaderStorage::execSelectMsgInfo", Logger::L_Info,
                            "Calling getMsgInfo for key <%s> returned pointer to %p\n",
                            (const char*) (*pKeys)[i], pMI);
            pRet->append (pMI);
        }
    }

    delete pKeys;
    pKeys = NULL;
    return pRet;
}

PtrLList<MessageHeader> * SQLMessageHeaderStorage::execSelectMsgInfo (const char *pszSQLStmt, uint16 ui16Limit)
{
    _m.lock (218);
    DArray2<String> *pKeys = execQueryAndReturnKey (pszSQLStmt, ui16Limit);
    _m.unlock (218);

    if (pKeys == NULL || pKeys->getSize() == 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::execSelectMsgInfo", Logger::L_Info,
                        "execSelectID returned NULL.\n");
        return NULL;
    }

    PtrLList<MessageHeader> *pRet = new PtrLList<MessageHeader>();

    MessageHeader *pMI;
    for (unsigned int i = 0; i < pKeys->size(); i++) {
        if (pKeys->used (i)) {
            pMI = getMsgInfo ((*pKeys)[i]);
            checkAndLogMsg ("SQLMessageHeaderStorage::execSelectMsgInfo", Logger::L_Info,
                            "Calling getMsgInfo for key <%s> returned pointer to %p\n",
                           (const char*) (*pKeys)[i], pMI);
            pRet->append (pMI);
        }
    }

    delete pKeys;
    pKeys = NULL;
    return pRet;
}

DArray2<String> * SQLMessageHeaderStorage::execSelectID (const char *pszGroupName, const char *pszSenderNodeId, uint8 ui8ClientType,
                                                         uint16 ui6Tag, uint32 ui32From, uint32 ui32To, uint32 ui32OffsetStart, uint32 ui32OffsetEnd,
                                                         uint16 ui16Limit, const char * pszOrderBy, bool bDescOrder, DArray2<String> *pDAArgs)
{
    return execSelectQuery ((const char *)PRIMARY_KEY, pszGroupName,
                            pszSenderNodeId, ui8ClientType, ui6Tag, ui32From, ui32To,
                            ui32OffsetStart, ui32OffsetEnd, ui16Limit, pszOrderBy,
                            bDescOrder, pDAArgs);
}

DArray2<String> * SQLMessageHeaderStorage::execSender (const char *pszGroupName, const char *pszSenderNodeId, uint8 ui8ClientType, uint16 ui6Tag,
                                                       uint32 ui32From, uint32 ui32To, uint32 ui32OffsetStart, uint32 ui32OffsetEnd, uint16 ui16Limit,
                                                       const char * pszOrderBy, bool bDescOrder, DArray2<String> *pDAArgs)
{
    DArray2<String> *pResults = NULL;

    pResults = execSelectQuery (FIELD_SENDER_ID, pszGroupName, pszSenderNodeId,
                                ui8ClientType, ui6Tag, ui32From, ui32To, ui32OffsetStart, ui32OffsetEnd,
                                ui16Limit, pszOrderBy, bDescOrder, pDAArgs);
    if (pResults != NULL) {
        DArray2<String> tokens;
        for (unsigned int i = 0; i < pResults->size(); i++) {
            if (pResults->used (i)) {
                if (0 == convertKeyToField ((const char *) ((*pResults)[i]), tokens, 1, MSG_ID_SENDER)) {
                   (*pResults)[i] = tokens[MSG_ID_SENDER];
                }
                else {
                    break;
                }
            }
        }
    }
    return pResults;
}

DArray2<String> * SQLMessageHeaderStorage::getSenders (const char *pszGroupName, uint16 ui16Limit)
{
    String sql = "SELECT DISTINCT ";
    sql = sql + FIELD_SENDER_ID + " FROM ";
    sql = sql + TABLE_NAME + " WHERE ";
    sql = sql + FIELD_GROUP_NAME + " = ?;";

    _m.lock (219);
    PreparedStatement *pStmt = _pDB->prepare ((const char *) sql);
    if (pStmt == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getSenders", Logger::L_SevereError,
                        "could not prepare statement\n");
        _m.unlock (219);
        return NULL;
    }

    if (pStmt->bind (1, pszGroupName) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getSenders", bindingError);
        pStmt->reset();
        delete pStmt;
        _m.unlock (219);
        return NULL;
    }

    Row *pRow = pStmt->getRow();
    if (pRow == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getSenders", memoryExhausted);
        delete pStmt;
        _m.unlock (219);
        return NULL;
    }

    if (!pStmt->next (pRow)) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getSenders", Logger::L_Info,
                        "the query did not return any result\n");
        delete pStmt;
        delete pRow;
        _m.unlock (219);
        return NULL;
    }

    DArray2<String> *pSenders = new DArray2<String>();
    unsigned int i = 0;
    do {
        char *pszSenderNodeId;
        if (pRow->getValue (0, &pszSenderNodeId) == 0) {
            (*pSenders)[i] = pszSenderNodeId;
        }
        free (pszSenderNodeId);
        i++;
    } while (pStmt->next (pRow));

    delete pStmt;
    delete pRow;

    _m.unlock (219);

    if (!pSenders->used (0)) {
        delete pSenders;
        pSenders = NULL;
    }

    return pSenders;
}

DArray2<String> * SQLMessageHeaderStorage::execSelectQuery (const char *pszWhat, const char *pszGroupName, const char *pszSenderNodeId, uint8 ui8ClientType,
                                                            uint16 ui6Tag, uint32 ui32From, uint32 ui32To, uint32 ui32OffsetStart, uint32 ui32OffsetEnd,
                                                            uint16 ui16Limit, const char * pszOrderBy, bool bDescOrder, DArray2<String> *pDAArgs)
{
    String what (pszWhat);
//    for (int = 0; i < )
//    hasField (const char * pszField);

    // generate the sql statement
    String sql = "SELECT ";
    sql = sql + what + " FROM ";
    sql = sql + TABLE_NAME;

    bool isFirstCond = false;

    if (NULL != pszGroupName) {
        sql = (String) sql + " WHERE " + FIELD_GROUP_NAME + " = '" + pszGroupName + "'";
        isFirstCond = true;
    }

    if (NULL != pszSenderNodeId) {
         if ((!isFirstCond)) {
             sql = (String) sql + " WHERE " + FIELD_SENDER_ID + " = '" + pszSenderNodeId + "'";
             isFirstCond = true;
         }
         else {
             sql = (String) sql + " AND " + FIELD_SENDER_ID + " = '" + pszSenderNodeId + "'";
         }
    }

    char buf[12];
    if (0 != ui8ClientType) {
         if ((!isFirstCond)) {
             sql = (String) sql + " WHERE " + FIELD_CLIENT_TYPE + " = " + itoa(buf, ui8ClientType);
             isFirstCond = true;
         }
         else {
             sql = (String) sql + " AND " + FIELD_CLIENT_TYPE + " = " + itoa(buf, ui8ClientType);
         }
    }

    if (0 != ui6Tag) {
         if ((!isFirstCond)) {
             sql = (String) sql + " WHERE " + FIELD_TAG + " = " + itoa(buf, ui6Tag);
             isFirstCond = true;
         }
         else {
             sql = (String) sql + " AND " + FIELD_TAG + " = " + itoa(buf, ui6Tag);
         }
    }

    if (ui32From < ui32To) {
        String rangeStatement;
        rangeStatement = (String) FIELD_MSG_SEQ_ID + " BETWEEN " + itoa(buf, ui32From);
        rangeStatement = (String) rangeStatement + " AND " + itoa(buf, ui32To);
        if ((!isFirstCond)) {
            sql = (String) sql + " WHERE (" + rangeStatement + ")";
            isFirstCond = true;
        }
        else {
            sql = (String) sql + " AND (" + rangeStatement + ")";
        }
    }
    else if (ui32From == ui32To) {
         if ((!isFirstCond)) {
             sql = (String) sql + " WHERE " + FIELD_MSG_SEQ_ID + " = " + itoa(buf, ui32From);
             isFirstCond = true;
         }
         else {
             sql = (String) sql + " AND " + FIELD_MSG_SEQ_ID + " = " + itoa(buf, ui32From);
         }
    }

    if (ui32OffsetStart < ui32OffsetEnd) {
        String sOffsetStart = itoa (buf, ui32OffsetStart);
        String sOffsetEnd = itoa (buf, ui32OffsetEnd);
        String rangeStatement = (String)
                                "("
                              +      FIELD_FRAGMENT_OFFSET + " >= " + sOffsetStart + " AND " + FIELD_FRAGMENT_OFFSET + " < " + sOffsetEnd
                              + ")"
                              + " OR "
                              + "("
                              +     "(" + FIELD_FRAGMENT_OFFSET + " + " + FIELD_FRAGMENT_LENGTH + ") > " + sOffsetStart
                              +     " AND "
                              +     "(" + FIELD_FRAGMENT_OFFSET + " + " + FIELD_FRAGMENT_LENGTH + ") <= " + sOffsetEnd
                              + ")"
                              + " OR "
                              + "("
                              +     FIELD_FRAGMENT_OFFSET + " <= " + sOffsetStart
                              +     " AND "
                              +     "(" + FIELD_FRAGMENT_OFFSET + " + " + FIELD_FRAGMENT_LENGTH + ") >= " + sOffsetEnd
                              + ")";

        if ((!isFirstCond)) {
            sql = (String) sql + " WHERE (" + rangeStatement + ")";
            isFirstCond = true;
        }
        else {
            sql = (String) sql + " AND (" + rangeStatement + ")";
        }
    }

    isFirstCond = true;
    if ((pDAArgs != NULL) && (pDAArgs->size() > 0)) {
        sql = (String) sql + " GROUP BY ";                
        for (unsigned int i=0; i < pDAArgs->size(); i++) {
            if (pDAArgs->used(i) == 0) {
                continue;
            }
            if (isFirstCond) {
                sql = sql + (*pDAArgs)[i];
                isFirstCond = false;
            }
            sql = (String) sql + ", " + (*pDAArgs)[i];
        }
    }

    if (pszOrderBy != NULL) {
        sql = (String) sql + " ORDER BY " + pszOrderBy;
        if (bDescOrder) {
            sql = (String) sql + " DESC";
        }
        else {
            sql = (String) sql + " ASC";
        }
    }

    sql = sql + ";";

    _m.lock (220);
    DArray2<String> *pResult = execQueryAndReturnKey (sql, ui16Limit);
    _m.unlock (220);
    return pResult;
}

DArray2<String> * SQLMessageHeaderStorage::execSelectID (const char *pszSQLStatement, uint16 ui16Limit)
{
    return execQueryAndReturnKey (pszSQLStatement, ui16Limit);
}

DArray2<String> * SQLMessageHeaderStorage::execQueryAndReturnKey (const char *pszSQLStmt, uint16 ui16Limit)
{
    const char *pszMethodName = "SQLMessageHeaderStorage::execQueryAndReturnKey";

    _m.lock (221);

    PreparedStatement *pStmt = _pDB->prepare (pszSQLStmt);
    if (pStmt == NULL) {
        checkAndLogMsg (pszMethodName , Logger::L_SevereError,
                        "could not prepare statement\n");
        _m.unlock (221);
        return NULL;
    }

    Row *pRow = pStmt->getRow();
    if (pRow == NULL) {
        checkAndLogMsg (pszMethodName , memoryExhausted);
        delete pStmt;
        _m.unlock (221);
        return NULL;
    }

    if (!pStmt->next (pRow)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "found 0 matches for query <%s>\n", pszSQLStmt);
        delete pStmt;
        delete pRow;
        _m.unlock (221);
        return NULL;
    }

    DArray2<String> *pResultArray = new DArray2<String>();
    // A limit of 0 means that there are no constraints on the number of results.
    uint16 ui16 = 0;
    do {
        char *pszMsgId = (char *) getId (pRow);
        if (pszMsgId == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "could not create id\n");
            break;
        }
        (*pResultArray)[ui16] = pszMsgId;  // string makes a copy of pszMsgId
        free (pszMsgId);
        ui16++;
    } while (pStmt->next (pRow) && ((ui16Limit == 0) || (ui16 < ui16Limit)));

    delete pStmt;
    delete pRow;

    _m.unlock (221);
    return pResultArray;
}

//------------------------------------------------------------------------------
// ELIMINATE
//------------------------------------------------------------------------------

DArray2<String> * SQLMessageHeaderStorage::getExpiredEntries()
{
    char buffer[22];
    char * currentTime = i64toa (buffer, getTimeInMilliseconds());

    String sql ="SELECT ";
    sql += PRIMARY_KEY;
    sql += " FROM ";
    sql += TABLE_NAME;
    sql += " WHERE (";
    sql += FIELD_EXPIRATION;
    sql +=" <= (";
    sql += currentTime;
    sql += " - ";
    sql += FIELD_ARRIVAL_TIMESTAMP;
    sql += "))";
    sql += " AND ";
    sql += FIELD_EXPIRATION;
    sql += " <> 0;";
    checkAndLogMsg ("SQLMessageHeaderStorage::getExpiredEntry", Logger::L_Info,
                    "The query is %s\n", (const char *) sql);

    _m.lock (223);
    DArray2<String> *dArrRet = execQueryAndReturnKey (sql);
    _m.unlock (223);
    return dArrRet;
}

int SQLMessageHeaderStorage::eliminateAllTheMessageFragments (const char *pszGroupName, const char *pszSenderNodeId,
                                                              uint32 ui32MsgSeqId, uint8 ui8ChunkId,
                                                              NOMADSUtil::DArray2<NOMADSUtil::String> *pDeleteMessageIDs)
{
    _m.lock (224);
    if (pDeleteMessageIDs != NULL && _pEliminateFragmentIDs == NULL) {
        String sql;
        sql = "SELECT * FROM ";
        sql = sql + TABLE_NAME + " WHERE ";
        sql = sql + FIELD_GROUP_NAME + " = ?1";
        sql = sql + " AND ";
        sql = sql + FIELD_SENDER_ID + " = ?2";
        sql = sql + " AND ";
        sql = sql + FIELD_MSG_SEQ_ID + " = ?3";
        sql = sql + " AND ";
        sql = sql + FIELD_CHUNK_ID + " = ?4";
        sql = sql + " AND ";
        sql = sql + FIELD_TOT_MSG_LENGTH + " > ";
        sql = sql + FIELD_FRAGMENT_LENGTH;
        sql = sql + ";";

        _pEliminateFragmentIDs = _pDB->prepare ((const char *) sql);
        if (_pEliminateFragmentIDs == NULL) {
            checkAndLogMsg ("SQLMessageHeaderStorage::eliminateAllTheMessageFragments", Logger::L_SevereError,
                            "could not prepare statement for query is %s\n", (const char *) sql);
            _m.unlock (224);
            return -1;
        }
        else {
            checkAndLogMsg ("SQLMessageHeaderStorage::eliminateAllTheMessageFragments", Logger::L_SevereError,
                            "statement %s prepared successfully\n", (const char *) sql);
        }
    }
    if (pDeleteMessageIDs != NULL) {
         if (_pEliminateFragmentIDs->bind (1, pszGroupName) < 0 ||
             _pEliminateFragmentIDs->bind (2, pszSenderNodeId) < 0 ||
             _pEliminateFragmentIDs->bind (3, ui32MsgSeqId) < 0 ||
             _pEliminateFragmentIDs->bind (4, ui8ChunkId) < 0) {
            checkAndLogMsg ("SQLMessageHeaderStorage::eliminateAllTheMessageFragments",
                            Logger::L_SevereError, "binding error\n");
            _pEliminateFragmentIDs->reset();
            _m.unlock (224);
            return -2;
        }

        Row *pRow = _pEliminateFragmentIDs->getRow();
        if (_pEliminateFragmentIDs->next (pRow)) {
            // Build id and fill pDeleteMessageIDs
        }
        delete pRow;
        _pEliminateFragmentIDs->reset();
    }

    if (_pEliminateFragments == NULL) {
        String sql;
        sql = "DELETE FROM ";
        sql = sql + TABLE_NAME + " WHERE ";
        sql = sql + FIELD_GROUP_NAME + " = ?1";
        sql = sql + " AND ";
        sql = sql + FIELD_SENDER_ID + " = ?2";
        sql = sql + " AND ";
        sql = sql + FIELD_MSG_SEQ_ID + " = ?3";
        sql = sql + " AND ";
        sql = sql + FIELD_CHUNK_ID + " = ?4";
        sql = sql + " AND ";
        sql = sql + FIELD_TOT_MSG_LENGTH + " > ";
        sql = sql + FIELD_FRAGMENT_LENGTH;
        sql = sql + ";";

        _pEliminateFragments = _pDB->prepare ((const char *) sql);
        if (_pEliminateFragments == NULL) {
            checkAndLogMsg ("SQLMessageHeaderStorage::eliminateAllTheMessageFragments", Logger::L_SevereError,
                            "could not prepare statement for query is %s\n", (const char *) sql);
            _m.unlock (224);
            return -3;
        }
        else {
            checkAndLogMsg ("SQLMessageHeaderStorage::eliminateAllTheMessageFragments", Logger::L_SevereError,
                            "statement %s prepared successfully\n", (const char *) sql);
        }
    }

    if (_pEliminateFragments->bind (1, pszGroupName) < 0 ||
        _pEliminateFragments->bind (2, pszSenderNodeId) < 0 ||
        _pEliminateFragments->bind (3, ui32MsgSeqId) < 0 ||
        _pEliminateFragments->bind (4, ui8ChunkId) < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::eliminateAllTheMessageFragments",
                        Logger::L_SevereError, "binding error\n");
        _pEliminateFragments->reset();
        _m.unlock (224);
        return -4;
    }

    if (_pEliminateFragments->update() < 0) {
        checkAndLogMsg ("SQLMessageHeaderStorage::eliminateAllTheMessageFragments",
                        Logger::L_HighDetailDebug, "the update failed\n");
        _pEliminateFragments->reset();
        _m.unlock (224);
        return -5;
    }

    _pEliminateFragments->reset();
    _m.unlock (224);
    return 0;
}

/*!!*/ // Fix this method to also handle keys that are not fully qualified
int SQLMessageHeaderStorage::eliminate (const char *pszKey)
{
    const char *pszMethodName = "SQLMessageHeaderStorage::eliminate";
    if (pszKey == NULL) {
        return -1;
    }

    // tokenize the key
    DArray2<String> aTokenizedKey;
    convertKeyToField(pszKey, aTokenizedKey);
    if (aTokenizedKey.size() != 6) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "The tokenized key "
                        " contains %d tokens whe it should contain 5.\n",
                        aTokenizedKey.size());
        return -2;
    }

    _m.lock (225);
    // build the sql statement
    if (_pDeleteRow == NULL) {
        String sql;
        sql = "DELETE FROM ";
        sql = sql + TABLE_NAME + " WHERE ";
        sql = sql + FIELD_GROUP_NAME + " = ?1";
        sql = sql + " AND ";
        sql = sql + FIELD_SENDER_ID + " = ?2";
        sql = sql + " AND ";
        sql = sql + FIELD_MSG_SEQ_ID + " = ?3";
        sql = sql + " AND ";
        sql = sql + FIELD_CHUNK_ID + " = ?4";
        sql = sql + " AND ";
        sql = sql + FIELD_FRAGMENT_OFFSET + " = ?5";
        sql = sql + " AND ";
        sql = sql + FIELD_FRAGMENT_LENGTH + " = ?6";
        sql = sql + ";";

        _pDeleteRow = _pDB->prepare ((const char *)sql);
        if (_pDeleteRow == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "failed to prepare statement %s\n", (const char *) sql);
            _m.unlock (225);
            return -3;
        }
    }

    if (_pDeleteRow->bind (1, aTokenizedKey[0]) < 0 ||  // group name
        _pDeleteRow->bind (2, aTokenizedKey[1]) < 0 ||  // sender node id
        _pDeleteRow->bind (3, atoui32 (aTokenizedKey[2])) < 0 ||  // message seq id
        _pDeleteRow->bind (4, (uint8)atoui32 (aTokenizedKey[3])) < 0 ||  // chunk id
        _pDeleteRow->bind (5, atoui32 (aTokenizedKey[4])) < 0 ||  // fragment offset
        _pDeleteRow->bind (6, atoui32 (aTokenizedKey[5])) < 0) {  //fragment length
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "binding failed\n");
        _pDeleteRow->reset();
        _m.unlock (225);
        return -4;
    }

    int rc = _pDeleteRow->update();
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "update failed\n");
    }
    _pDeleteRow->reset();

    _m.unlock (225);
    return rc;
}

NOMADSUtil::PtrLList<MessageId> * SQLMessageHeaderStorage::getMessageIdsForFragments (void)
{
    int rc;
    _m.lock (226);
    if (_pFindAllMessageIDsWithFragments == NULL) {
        String sql;
        sql = "SELECT ";
        sql += FIELD_GROUP_NAME;
        sql += ",";
        sql += FIELD_SENDER_ID;
        sql += ",";
        sql += FIELD_MSG_SEQ_ID;
        sql += ",";
        sql += FIELD_CHUNK_ID;
        sql += " FROM ";
        sql += TABLE_NAME;
        sql += " WHERE ";
        sql += FIELD_FRAGMENT_LENGTH;
        sql += " != ";
        sql += FIELD_TOT_MSG_LENGTH;
        sql += ";";

        _pFindAllMessageIDsWithFragments = _pDB->prepare (sql);
        if (_pFindAllMessageIDsWithFragments == NULL) {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMessageHeadersForFragments", Logger::L_MildError,
                            "could not prepare statement for query <%s>\n", (const char *) sql);
            _m.unlock (226);
            return NULL;
        }
        else {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMessageHeadersForFragments", Logger::L_MildError,
                            "statement <%s> prepared successfully\n", (const char *) sql);
        }
    }
    Row *pRow = _pFindAllMessageIDsWithFragments->getRow();
    if (pRow == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMessageHeadersForFragments", Logger::L_MildError,
                        "failed to get Row\n");
        _m.unlock (226);
        return NULL;
    }
    PtrLList<MessageId> *pList = new PtrLList<MessageId>;
    while (_pFindAllMessageIDsWithFragments->next (pRow)) {
        char *pszGroupName = NULL;
        char *pszOriginatorNodeId = NULL;
        uint32 ui32MessageSeqId = 0;
        uint8 ui8ChunkId = 0;
        if (0 != (rc = pRow->getValue (0, &pszGroupName))) {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMessageHeadersForFragments", Logger::L_MildError,
                            "pRow->getValue() failed for column 0 with rc = %d\n", rc);
            delete pRow;
            _m.unlock (226);
            return pList;
        }
        if (0 != (rc = pRow->getValue (1, &pszOriginatorNodeId))) {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMessageHeadersForFragments", Logger::L_MildError,
                            "pRow->getValue() failed for column 0 with rc = %d\n", rc);
            delete pRow;
            free (pszGroupName);
            _m.unlock (226);
            return pList;
        }
        if (0 != (rc = pRow->getValue (2, ui32MessageSeqId))) {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMessageHeadersForFragments", Logger::L_MildError,
                            "pRow->getValue() failed for column 0 with rc = %d\n", rc);
            free (pszGroupName);
            free (pszOriginatorNodeId);
            _m.unlock (226);
            return pList;
        }
        if (0 != (rc = pRow->getValue (3, ui8ChunkId))) {
            checkAndLogMsg ("SQLMessageHeaderStorage::getMessageHeadersForFragments", Logger::L_MildError,
                            "pRow->getValue() failed for column 0 with rc = %d\n", rc);
            free (pszGroupName);
            free (pszOriginatorNodeId);
            _m.unlock (226);
            return pList;
        }
        MessageId *pMId = new MessageId (pszGroupName, pszOriginatorNodeId, ui32MessageSeqId, ui8ChunkId);
        pList->append (pMId);
    }
    delete pRow;
    _pFindAllMessageIDsWithFragments->reset();
    return pList;
}

char ** SQLMessageHeaderStorage::getDisseminationServiceIds (const char *pszObjectId, const char *pszInstanceId)
{
    int rc;
    _m.lock (226);
    
    PreparedStatement *pStmp = (pszInstanceId == NULL ? _pGetDSIdsByObjId : _pGetDSIdsByObjAndInstId);
    if (pStmp == NULL) {
        String sql;
        sql = "SELECT ";
        sql += FIELD_GROUP_NAME;
        sql += ",";
        sql += FIELD_SENDER_ID;
        sql += ",";
        sql += FIELD_MSG_SEQ_ID;
        sql += " FROM ";
        sql += TABLE_NAME;
        sql += " WHERE ";
        sql += FIELD_OBJECT_ID;
        sql += " = ?";
        if (pszInstanceId != NULL) {
            sql += " AND ";
            sql += FIELD_INSTANCE_ID;
            sql += " = ?";
            sql += ";";

            _pGetDSIdsByObjAndInstId = _pDB->prepare (sql);
            pStmp = _pGetDSIdsByObjAndInstId;
        }
        else {
            sql += ";";

            _pGetDSIdsByObjId = _pDB->prepare (sql);
            pStmp = _pGetDSIdsByObjId;
        }

        if (pStmp == NULL) {
            checkAndLogMsg ("SQLMessageHeaderStorage::getDisseminationServiceIds", Logger::L_MildError,
                            "could not prepare statement for query <%s>\n", (const char *) sql);
            _m.unlock (226);
            return NULL;
        }
        else {
            checkAndLogMsg ("SQLMessageHeaderStorage::getDisseminationServiceIds", Logger::L_Info,
                            "statement <%s> prepared successfully\n", (const char *) sql);
        }
    }

    pStmp->reset();
    pStmp->bind (1, pszObjectId);
    if (pszInstanceId != NULL) {
        pStmp->bind (2, pszInstanceId);
    }

    Row *pRow = pStmp->getRow();
    if (pRow == NULL) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getDisseminationServiceIds", Logger::L_Warning,
                        "failed to get Row\n");
        _m.unlock (226);
        return NULL;
    }

    DArray2<String> ret;
    int i = 0;
    while (pStmp->next (pRow)) {
        char *pszGroupName, *pszPublisherNodeId, *pszMsgSeqId;
        pszGroupName = pszPublisherNodeId =  pszMsgSeqId = NULL;
        
        if (((rc = pRow->getValue (0, &pszGroupName)) == 0) &&
            ((rc = pRow->getValue (1, &pszPublisherNodeId)) == 0) &&
            ((rc = pRow->getValue (2, &pszMsgSeqId)) == 0)) {

            char *pszTmp = convertFieldToKey (pszGroupName, pszPublisherNodeId, pszMsgSeqId);
            if (pszTmp != NULL) {
                ret[i] = pszTmp;
                free (pszTmp);
                i++;
            }
        }
        else {
            checkAndLogMsg ("SQLMessageHeaderStorage::getDisseminationServiceIds", Logger::L_MildError,
                            "pRow->getValue() failed with rc = %d\n", rc);
        }
    }

    _m.unlock (226);

    if (ret.size() <= 0) {
        return NULL;
    }
    char **ppszRet = (char **) calloc (ret.size()+1, sizeof (char *));
    if (ppszRet == NULL) {
        return NULL;
    }

    for (unsigned int i = 0; i < ret.size(); i++) {
        ppszRet[i] = ret[i].r_str();
    }

    return ppszRet;
}

DArray2<String> * SQLMessageHeaderStorage::getResourceOrderedByUtility()
{
    // SELECT * AND ORDER BY USEFULNESS
    String sql = "SELECT ";
    sql = sql + PRIMARY_KEY + " FROM ";
    sql = sql + TABLE_NAME + " ORDER BY ";
    //sql = sql + FIELD_PRIORITY + ";";
    sql = sql + FIELD_ARRIVAL_TIMESTAMP + " DESC;";

    checkAndLogMsg ("SQLMessageHeaderStorage::getResourceOrderedByUtility",
                    Logger::L_Info, "The query is %s\n", (const char *) sql);

    _m.lock (227);
    DArray2<String> *dArrRet = execQueryAndReturnKey (sql);
    _m.unlock (227);
    return dArrRet;
}

