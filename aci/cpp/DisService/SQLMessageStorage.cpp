/*
 * SQLMessageStorage.cpp
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

#include "SQLMessageStorage.h"

// milliseconds
#define TRANSACTION_COMMIT_INTERVAL 60000
// iterations
#define VACUUM_INTERVAL 10

#include "DisServiceDataCacheQuery.h"
#include "DisServiceDefs.h"
#include "DSSFLib.h"
#include "Message.h"
#include "MessageInfo.h"
#include "SQLMessageHeaderStorage.h"

#include "PreparedStatement.h"

#include "Logger.h"
#include "NLFLib.h"
#include "StrClass.h"

using namespace IHMC_ACI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

//==============================================================================
//  SQLITE INTERFACE
//==============================================================================

SQLMessageStorage::SQLMessageStorage (const char *pszDBName, bool bUseTransactionTimer)
    : SQLMessageHeaderStorage (pszDBName),
    _pCommitThread (NULL),
    _bUseTransactionTimer (pszDBName == NULL ? false : bUseTransactionTimer),
    _pDSDCQuery (new DisServiceDataCacheQuery ()),
    _pGetData (NULL),
    _pGetFullyQualifiedMsg (NULL),
    _pGetMsg (NULL),
    _pGetComplChunksPrepStmt (NULL),
    _pGetComplAnnotationsPrepStmt (NULL)
{
}

SQLMessageStorage::~SQLMessageStorage (void)
{
    delete _pDSDCQuery;
    _pDSDCQuery = NULL;
    delete _pGetData;
    _pGetData = NULL;
    delete _pGetFullyQualifiedMsg;
    _pGetFullyQualifiedMsg = NULL;
    delete _pGetComplChunksPrepStmt;
    _pGetComplChunksPrepStmt = NULL;
    delete _pGetComplAnnotationsPrepStmt;
    _pGetComplAnnotationsPrepStmt = NULL;
    if (_pCommitThread != NULL && _pCommitThread->isRunning()) {
        _pCommitThread->requestTermination();
    }
}

int SQLMessageStorage::init()
{
    if (SQLMessageHeaderStorage::init() < 0) {
        return -1;
    }

    const char *pszMethodName = "SQLMessageStorage::init";

    // start a transaction. jk 12/2009
    // Now we can set whether or not we are using transactions using a setting in the configuration file.
    // Eventually we should probably fix it so the transaction timer interval is configurable as well.
    // jk 8/2010
    if (_bUseTransactionTimer) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Using transaction timer\n");
        if (_pDB->execute ("BEGIN;") < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Can't begin transaction\n");
            return -9;
        }

        _pCommitThread = new CommitThread (this);
        _pCommitThread->start();
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Not using transaction timer\n");
    }

    return 0;
}

String SQLMessageStorage::getCreateTableSQLStatement()
{
    String sql = (String) "CREATE TABLE IF NOT EXISTS " + SQLMessageHeaderStorage::TABLE_NAME + " (";

    sql = sql + FIELD_GROUP_NAME + " TEXT, ";
    sql = sql + FIELD_SENDER_ID + " TEXT, ";
    sql = sql + FIELD_MSG_SEQ_ID + " INT, ";

    sql = sql + FIELD_CHUNK_ID + " INT, ";

    sql = sql + FIELD_OBJECT_ID + " TEXT, ";
    sql = sql + FIELD_INSTANCE_ID + " TEXT, ";
    sql = sql + FIELD_ANNOTATION_MSG_ID + " TEXT, ";
    sql = sql + FIELD_ANNOTATION_METADATA + " BLOB, ";

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
    sql = sql + FIELD_DATA +" BLOB, ";

    sql += "PRIMARY KEY (";
    sql = sql + SQLMessageHeaderStorage::PRIMARY_KEY + "));";

    return sql;
}

String SQLMessageStorage::getInsertIntoTableSQLStatement()
{
    String sql = "INSERT INTO ";
    sql = sql + SQLMessageHeaderStorage::TABLE_NAME + " (";
    sql = sql + SQLMessageHeaderStorage::ALL_PERSISTENT + ")";
    sql = sql + " VALUES (?,?,?,?,?,"
                         "?,?,?,?,?,"
                         "?,?,?,?,?,"
                         "?,?,?,?,?,"
                         "?,?,?,?,?);";

    return sql;
}

int SQLMessageStorage::insertIntoDataCacheBind (PreparedStatement *pStmt, Message *pMsg)
{
    if (SQLMessageHeaderStorage::insertIntoDataCacheBind (pStmt, pMsg) < 0) {
        return -1;
    }

    return pStmt->bind (SQLMessageHeaderStorage::FIELD_DATA_COLUMN_NUMBER + 1,
                        pMsg->getData(), pMsg->getMessageHeader()->getFragmentLength());
}

void * SQLMessageStorage::getData (const char *pszKey)
{
    const char *pszMethodName = "SQLMessageStorage::getData";
    if (pszKey == NULL) {
        return NULL;
    }

    _m.lock (228);
    if (_pGetData == NULL) {
        String sql = (String) "SELECT " + SQLMessageHeaderStorage::FIELD_DATA
                   + " FROM  " + SQLMessageHeaderStorage::TABLE_NAME + " WHERE "
                   + SQLMessageHeaderStorage::FIELD_GROUP_NAME + " = ?1 AND "
                   + SQLMessageHeaderStorage::FIELD_SENDER_ID + " = ?2 AND "
                   + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " = ?3 AND "
                   + SQLMessageHeaderStorage::FIELD_CHUNK_ID + " = ?4 AND "
                   + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET +" = ?5 AND "
                   + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH + " = ?6;";

        _pGetData = _pDB->prepare ((const char *)sql);
        if (_pGetData == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "failed to prepare statement. %s\n", (const char *)sql);
            _m.unlock (228);
            return NULL;
        }
    }

    DArray2<NOMADSUtil::String> tokens (6);
    if (convertKeyToField (pszKey, tokens) != 0) {
        _m.unlock (228);
        return NULL;
    }

    if (_pGetData->bind (1, tokens[MSG_ID_GROUP]) < 0 ||
        _pGetData->bind (2, tokens[MSG_ID_SENDER]) < 0 ||
        _pGetData->bind (3, atoi (tokens[MSG_ID_SEQ_NUM])) < 0 ||
        _pGetData->bind (4, atoi (tokens[MSG_ID_CHUNK_ID])) < 0 ||
        _pGetData->bind (5, atoi (tokens[MSG_ID_OFFSET])) < 0 ||
        _pGetData->bind (6, atoi (tokens[MSG_ID_LENGTH])) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Error when binding values.\n");
        _pGetData->reset();
        _m.unlock (228);
        return NULL;
    }
    
    Row *pRow = _pGetData->getRow();
    if (pRow == NULL) {
        checkAndLogMsg ("SQLMessageStorage::getData", memoryExhausted);
        _pGetData->reset();
        _m.unlock (228);
        return NULL;
    }

    void *pRet = NULL;
    int iLen;
    for (unsigned short i = 0; _pGetData->next (pRow); i++) {
        if (i > 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "Message is not uniquely identified.\n");
        }
        assert (i == 0);
        if (pRow->getValue (0, &pRet, iLen) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "pRow->getValue returned an error.\n");
            break;
        }
    }

    delete pRow;
    _pGetData->reset();
    _m.unlock (228);

    return pRet;
}

Message * SQLMessageStorage::getMessage (const char *pszKey)
{
    const char *pszMethodName = "SQLMessageStorage::getMessage";

    if (pszKey == NULL) {
        return NULL;
    }

    DArray2<String> tokens (6);
    IHMC_MISC::PreparedStatement *pQuery = NULL;

    if (isFullyQualifiedMessageID (pszKey)) {
        if (convertKeyToField (pszKey, tokens) != 0) {
            return NULL;
        }
        _m.lock (229);
        if (_pGetFullyQualifiedMsg == NULL) {
            String sql = (String) "SELECT " + SQLMessageHeaderStorage::ALL_PERSISTENT
            + " FROM " + SQLMessageHeaderStorage::TABLE_NAME + " WHERE "
            + SQLMessageHeaderStorage::FIELD_GROUP_NAME + " = ?1 AND "
            + SQLMessageHeaderStorage::FIELD_SENDER_ID + " = ?2 AND "
            + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " = ?3 AND "
            + SQLMessageHeaderStorage::FIELD_CHUNK_ID + " = ?4 AND "
            + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET +" = ?5 AND "
            + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH + " = ?6;";
            _pGetFullyQualifiedMsg = _pDB->prepare ((const char *)sql);
            if (_pGetFullyQualifiedMsg == NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "failed to prepare statement: %s\n", (const char *)sql);
                _m.unlock (229);
                return NULL;
            }
        }
        if (_pGetFullyQualifiedMsg->bind (1, tokens[MSG_ID_GROUP]) < 0 ||
            _pGetFullyQualifiedMsg->bind (2, tokens[MSG_ID_SENDER]) < 0 ||
            _pGetFullyQualifiedMsg->bind (3, atoi (tokens[MSG_ID_SEQ_NUM])) < 0 ||
            _pGetFullyQualifiedMsg->bind (4, atoi (tokens[MSG_ID_CHUNK_ID])) < 0 ||
            _pGetFullyQualifiedMsg->bind (5, atoi (tokens[MSG_ID_OFFSET])) < 0 ||
            _pGetFullyQualifiedMsg->bind (6, atoi (tokens[MSG_ID_LENGTH])) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "error when binding values to do fully qualified message query\n");
            _pGetFullyQualifiedMsg->reset();
            _m.unlock (229);
            return NULL;
        }
        pQuery = _pGetFullyQualifiedMsg;
    }
    else {
        if (convertKeyToField (pszKey, tokens, 4, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM, MSG_ID_CHUNK_ID) != 0) {
            return NULL;
        }
        _m.lock (229);
        if (_pGetMsg == NULL) {
            String sql = (String) "SELECT " + SQLMessageHeaderStorage::ALL_PERSISTENT 
                         + " FROM " + SQLMessageHeaderStorage::TABLE_NAME + " WHERE "
                         + SQLMessageHeaderStorage::FIELD_GROUP_NAME + " = ?1 AND "
                         + SQLMessageHeaderStorage::FIELD_SENDER_ID + " = ?2 AND "
                         + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " = ?3 AND "
                         + SQLMessageHeaderStorage::FIELD_CHUNK_ID + " = ?4 AND "
                         + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET +" = 0 AND "
                         + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH + " = " + SQLMessageHeaderStorage::FIELD_TOT_MSG_LENGTH + ";";
            _pGetMsg = _pDB->prepare (sql);
            if (_pGetMsg == NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "failed to prepare statement: %s\n", sql.c_str());
                _m.unlock (229);
                return NULL;
            }
        }
        if (_pGetMsg->bind (1, tokens[MSG_ID_GROUP]) < 0 ||
            _pGetMsg->bind (2, tokens[MSG_ID_SENDER]) < 0 ||
            _pGetMsg->bind (3, atoi (tokens[MSG_ID_SEQ_NUM])) < 0 ||
            _pGetMsg->bind (4, atoi (tokens[MSG_ID_CHUNK_ID])) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "error when binding values to do message query\n");
            _pGetMsg->reset();
            _m.unlock (229);
            return NULL;
        }
        pQuery = _pGetMsg;
    }

    Row *pRow = pQuery->getRow();
    if (pRow == NULL) {
        pQuery->reset();
        _m.unlock (229);
        return NULL;
    }

    void *pData = NULL;
    MessageHeader *pMI = NULL;
    int iLen;
    for (unsigned short i = 0; pQuery->next (pRow); i++) {
        if (i > 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "message <%s> is not uniquely identified\n", pszKey);
            delete pMI;
            delete pRow;
            return NULL;
        }

        pMI = getMessageInfo (pRow);
        if (pRow->getValue (SQLMessageHeaderStorage::FIELD_DATA_COLUMN_NUMBER, &pData, iLen) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "pRow->getValue returned an error.\n");
            break;
        }
    }

    delete pRow;
    pQuery->reset();
    _m.unlock (229);

    if (pMI != NULL && pData != NULL) {
        return new Message (pMI, pData);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "\n");
    }
    delete pMI;
    free (pData);
    return NULL;
}

PtrLList<Message> * SQLMessageStorage::getMessages (DisServiceDataCacheQuery *pQuery)
{
    uint16 ui16Limit;
    if (pQuery->isLimitSet()) {
        ui16Limit = pQuery->getLimit();
    }
    else {
        ui16Limit = 65535;
    }

    _m.lock (230);
    PreparedStatement *pStmt = _pDB->prepare (pQuery->getSqlQuery());
    PtrLList<Message> *pRet = getMessages (pStmt, ui16Limit);
    pStmt->reset();
    delete pStmt;
    if (pRet != NULL && pRet->getFirst() == NULL) {
        delete pRet;
        pRet = NULL;
        checkAndLogMsg ("SQLMessageStorage::getMessages", Logger::L_Info,
                        "the query did not return any result: %s\n", pQuery->getSqlQuery());
    }
    _m.unlock (230);
    return pRet;
}

PtrLList<Message> * SQLMessageStorage::getCompleteChunks (const char *pszGroupName, const char *pszSender,
                                                          uint32 ui32MsgSeqId)
{
    _m.lock (231);
    if (_pGetComplChunksPrepStmt == NULL) {
        String sql = (String) "SELECT " + SQLMessageHeaderStorage::ALL_PERSISTENT + " FROM " + SQLMessageHeaderStorage::TABLE_NAME + " WHERE "
                                 + SQLMessageHeaderStorage::FIELD_GROUP_NAME + " = ?1 AND "
                                 + SQLMessageHeaderStorage::FIELD_SENDER_ID + " = ?2 AND "
                                 + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " = ?3 AND "
                                 + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH + " = " + SQLMessageHeaderStorage::FIELD_TOT_MSG_LENGTH + ";";

        _pGetComplChunksPrepStmt = _pDB->prepare (sql);
        if (_pGetComplChunksPrepStmt != NULL) {
            checkAndLogMsg ("SQLMessageStorage::getCompleteChunks", Logger::L_Info,
                            "Statement %s prepared successfully.\n", sql.c_str());
        }
        else {
            checkAndLogMsg ("SQLMessageStorage::getCompleteChunks", Logger::L_SevereError,
                            "Could not prepare statement: %s.\n", sql.c_str()); 
            _m.unlock (231);
            return NULL;
        }
    }

    // Bind the values to the prepared statement
    if (_pGetComplChunksPrepStmt->bind (1, pszGroupName) < 0 ||
        _pGetComplChunksPrepStmt->bind (2, pszSender) < 0 || 
        _pGetComplChunksPrepStmt->bind (3, ui32MsgSeqId) < 0) {
        checkAndLogMsg ("SQLMessageStorage::getCompleteChunks", bindingError);
        _pGetComplChunksPrepStmt->reset();
        _m.unlock (231);
        return NULL;
    }

    // Execute the statement
    PtrLList<Message> *pList = getMessages (_pGetComplChunksPrepStmt);
    _pGetComplChunksPrepStmt->reset();
    if (pList == NULL || pList->getFirst() == NULL) {
        checkAndLogMsg ("SQLMessageStorage::getCompleteChunks", Logger::L_HighDetailDebug,
                        "did not find any fragment matching %s:%s:%u\n", pszGroupName, pszSender,
                        ui32MsgSeqId);
    }

    _m.unlock (231);
    return pList;
}

PtrLList<Message> * SQLMessageStorage::getCompleteAnnotations (const char *pszAnnotatedObjMsgId)
{
    const char *pszMethodName = "SQLMessageStorage::getCompleteAnnotation";
    if (pszAnnotatedObjMsgId == NULL) {
        return NULL;
    }
    _m.lock (231);
    if (_pGetComplAnnotationsPrepStmt == NULL) {
        String sql = (String) "SELECT " + SQLMessageHeaderStorage::ALL_PERSISTENT + " FROM " + SQLMessageHeaderStorage::TABLE_NAME + " WHERE "
            + SQLMessageHeaderStorage::FIELD_ANNOTATION_MSG_ID + " = ?1 AND "
            + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH + " = " + SQLMessageHeaderStorage::FIELD_TOT_MSG_LENGTH + ";";

        _pGetComplAnnotationsPrepStmt = _pDB->prepare (sql.c_str ());
        if (_pGetComplAnnotationsPrepStmt != NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                "Statement %s prepared successfully.\n", sql.c_str ());
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                "Could not prepare statement: %s.\n", sql.c_str());
            _m.unlock (231);
            return NULL;
        }
    }

    // Bind the values to the prepared statement
    if (_pGetComplAnnotationsPrepStmt->bind (1, pszAnnotatedObjMsgId) < 0) {
        checkAndLogMsg (pszMethodName, bindingError);
        _pGetComplAnnotationsPrepStmt->reset ();
        _m.unlock (231);
        return NULL;
    }

    // Execute the statement
    PtrLList<Message> *pList = getMessages (_pGetComplAnnotationsPrepStmt);
    _pGetComplAnnotationsPrepStmt->reset();
    if (pList == NULL || pList->getFirst() == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                        "did not find any annotation for %s\n", pszAnnotatedObjMsgId);
    }

    _m.unlock (231);
    return pList;
}

PtrLList<Message> * SQLMessageStorage::getMessages (PreparedStatement *pStmt, uint16 ui16LimitElements)
{
    Row *pRow = pStmt->getRow();
    if (pRow == NULL) {
        return NULL;
    }
    if (!pStmt->next (pRow)) {
        delete pRow;
        return NULL;
    }

    PtrLList<Message> *pRet = new PtrLList<Message>();
    if (pRet == NULL) {
        checkAndLogMsg ("SQLMessageStorage::getMessages", memoryExhausted);
        delete pRow;
        return NULL;
    }

    uint16 ui16NElements = 0;
    do {
        MessageHeader *pMH = getMessageInfo (pRow);
        if (pMH == NULL) {
            continue;
        }
        void *pData = NULL;
        int iLen;
        int rc = pRow->getValue (SQLMessageHeaderStorage::FIELD_DATA_COLUMN_NUMBER,
                                 &pData, iLen);
        if (rc < 0) {
            delete pMH;
            continue;
        }

        Message *pMsg = new Message (pMH, pData);
        if (pMsg != NULL) {
            pRet->append (pMsg);
            ui16NElements++;
        }
        else {
            checkAndLogMsg ("SQLMessageStorage::getMessages", memoryExhausted);
        }
    } while ((pStmt->next (pRow)) &&
             ((ui16LimitElements == 0) || (ui16NElements < ui16LimitElements)));

    delete pRow;

    return pRet;
}

//------------------------------------------------------------------------------
// CommitThread
//------------------------------------------------------------------------------

// jk 12/2009
SQLMessageStorage::CommitThread::CommitThread (SQLMessageStorage *parent)
{
    this->parent = parent;
}

SQLMessageStorage::CommitThread::~CommitThread()
{
    // empty
}

// This thread commits the database transaction and starts a new transaction every 15 seconds
// to try and increase performance. A SQLite database update entails a file creation, a bunch
// of disk writes, three disk cache flushes, and a file delete, so it can be a very heavy
// operation especially if you are running on, say, a flash disk.
void SQLMessageStorage::CommitThread::run()
{
    const char *pszMethodName = "SQLMessageStorage::CommitThread::run";
    setName (pszMethodName);

    int s;
    int count = 0;

    started();

    while (!terminationRequested()) {
        sleepForMilliseconds (TRANSACTION_COMMIT_INTERVAL);

        parent->_m.lock (232);
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                        "Committing database\n");
        do {
            if ((s = parent->_pDB->endTransaction()) < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "Can't commit - error %d, retrying\n", s);
                sleepForMilliseconds (1000);
            }
        } while (s < 0);
        count++;

        if (count >= VACUUM_INTERVAL) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Vacuuming database\n");
            do {
                if ((s = parent->_pDB->execute ("VACUUM;")) < 0) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                    "Can't vacuum database - error %d, retrying\n", s);
                    sleepForMilliseconds (1000);
                }
            } while (s < 0);
            count = 0;
        }

        do {
            if ((s = parent->_pDB->beginTransaction()) < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "Can't begin transaction - error %d, retrying\n", s);
                sleepForMilliseconds (1000);
            }
        } while (s < 0);
        parent->_m.unlock (232);
    }

    // Commit before terminating!
    if ((s = parent->_pDB->endTransaction()) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Can't commit - error %d, retrying\n", s);
    }

    terminating();
}
