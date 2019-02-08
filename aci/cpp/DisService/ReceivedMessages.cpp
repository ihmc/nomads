/*
 * ReceivedMessages.cpp
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

#include "ReceivedMessages.h"
#include "DisServiceDefs.h"

#include "Database.h"
#include "PreparedStatement.h"
#include "Result.h"

#include "Logger.h"
#include "StrClass.h"

using namespace IHMC_ACI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

const NOMADSUtil::String ReceivedMessages::TABLE_GROUP_AND_PUBLISHER = "GroupAndPublisher";
const NOMADSUtil::String ReceivedMessages::GRP_NAME = "GroupName";
const NOMADSUtil::String ReceivedMessages::PUBLISHER_NODE_ID = "PublisherNodeId";
const NOMADSUtil::String ReceivedMessages::GROUP_AND_PUBLISHER_ROW_ID = "GroupAndPublisherId";

const NOMADSUtil::String ReceivedMessages::MESSAGE_SEQUENCE_ID_TABLE = "MessageSequenceIdTable";
const NOMADSUtil::String ReceivedMessages::MESSAGE_SEQUENCE_ID = "MessageSequenceId";
const NOMADSUtil::String ReceivedMessages::GROUP_AND_PUBLISHER_ID = "GroupAndPubId";

bool bind (const char *pszGroupName, const char *pszPublisherNodeId,
           uint32 ui32MsgSeqId, PreparedStatement *pStmt);
bool bind (const char *pszGroupName, const char *pszPublisherNodeId,
           PreparedStatement *pStmt);

ReceivedMessages::ReceivedMessages (const char *pszStorageFile)
{
    _pszStorageFile = pszStorageFile;
    construct();
}

ReceivedMessages::~ReceivedMessages()
{
    _pszStorageFile = NULL;
    if (_psqlInsertGrpPub != NULL) {
        delete _psqlInsertGrpPub;
        _psqlInsertGrpPub = NULL;
    }
    if (_psqlInsertMsgSeqId != NULL) {
        delete _psqlInsertMsgSeqId;
        _psqlInsertMsgSeqId = NULL;
    }
    if (_psqlSelectGrpPubSeqCount != NULL) {
        delete _psqlSelectGrpPubSeqCount;
        _psqlSelectGrpPubSeqCount = NULL;
    }
    if (_psqlSelectGrpPubSeq != NULL) {
        delete _psqlSelectGrpPubSeq;
        _psqlSelectGrpPubSeq = NULL;
    }
    if (_psqlSelectGrpPubSeqMax != NULL) {
        delete _psqlSelectGrpPubSeqMax;
        _psqlSelectGrpPubSeqMax = NULL;
    }
    if (_psqlSelectGrpPubSeqAll == NULL) {
        delete _psqlSelectGrpPubSeqAll;
        _psqlSelectGrpPubSeqAll = NULL;
    }
    if (_psqlSelectGrpPubRowId == NULL) {
        delete _psqlSelectGrpPubRowId;
        _psqlSelectGrpPubRowId = NULL;
    }
}

void ReceivedMessages::construct()
{
    _psqlInsertGrpPub = NULL;
    _psqlInsertMsgSeqId = NULL;
    _psqlSelectGrpPubSeqCount = NULL;
    _psqlSelectGrpPubSeqMax = NULL;
    _psqlSelectGrpPubSeq = NULL;
    _psqlSelectGrpPubSeqAll = NULL;
    _psqlSelectGrpPubRowId = NULL;
}

int64 ReceivedMessages::getGrpPubRowId (const char *pszGroupName,
                                        const char *pszPublisherNodeId)
{
    if (!bind (pszGroupName, pszPublisherNodeId, _psqlSelectGrpPubRowId)) {
        return -1;
    }
    Row *pRow = _psqlSelectGrpPubRowId->getRow();
    if (pRow == NULL) {
        _psqlSelectGrpPubRowId->reset();
        return -2;
    }

    int64 i64RowId = -1;
    if (_psqlSelectGrpPubRowId->next (pRow) &&
        pRow->getValue (0, i64RowId) < 0) {

        delete pRow;
        return -3;
    }
    _psqlSelectGrpPubRowId->reset();
    return i64RowId;
}

int ReceivedMessages::init()
{
    const char *pszMethodName = "ReceivedMessages::init";
    DatabasePtr *pDB = Database::getDatabase (Database::SQLite);
    if (pDB == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Database::getDatabase returned NULL pointer.\n");
        return -1;
    }
    if ((*pDB)->open (_pszStorageFile) != 0) {
        return -2;
    }

    _m.lock();

    //////////////////////// CREATE TABLES  //////////////////////////////

    String sql = (String) "CREATE TABLE IF NOT EXISTS " + TABLE_GROUP_AND_PUBLISHER +  "("
               +           GROUP_AND_PUBLISHER_ROW_ID + " INTEGER PRIMARY KEY, "
               +           GRP_NAME + " TEXT, "
               +           PUBLISHER_NODE_ID + " TEXT, "
               +           "UNIQUE (" + GRP_NAME + ", " + PUBLISHER_NODE_ID + "));";
    if ((*pDB)->execute (sql.c_str()) != 0) {
        _m.unlock();
        return -3;
    }

    sql = (String) "CREATE TABLE IF NOT EXISTS " + MESSAGE_SEQUENCE_ID_TABLE +  "("
               +    MESSAGE_SEQUENCE_ID + " INTEGER, "
               +    GROUP_AND_PUBLISHER_ID + " INTEGER, "
               +    "FOREIGN KEY (" + GROUP_AND_PUBLISHER_ID + ") REFERENCES " + TABLE_GROUP_AND_PUBLISHER + "(" + GROUP_AND_PUBLISHER_ROW_ID + "), "
               +    "UNIQUE (" + MESSAGE_SEQUENCE_ID + ", " + GROUP_AND_PUBLISHER_ID + "));";
    if ((*pDB)->execute (sql.c_str()) != 0) {
        _m.unlock();
        return -4;
    }

    ///////////////////////// CREATE INSERT STATEMENTS /////////////////////////

    sql = (String) "INSERT INTO " + TABLE_GROUP_AND_PUBLISHER
        +          " (" + GRP_NAME + "," + PUBLISHER_NODE_ID + ") VALUES (?,?);";
    _psqlInsertGrpPub = (*pDB)->prepare (sql.c_str());
    if (_psqlInsertGrpPub == NULL) {
        _m.unlock();
        return -5;
    }

    sql = (String) "INSERT INTO " + MESSAGE_SEQUENCE_ID_TABLE
        +          " (" + MESSAGE_SEQUENCE_ID + ", " + GROUP_AND_PUBLISHER_ID + ") VALUES (?,?);";
    _psqlInsertMsgSeqId = (*pDB)->prepare (sql.c_str());
    if (_psqlInsertMsgSeqId == NULL) {
        _m.unlock();
        return -6;
    }

    ///////////////////////// CREATE SELECT STATEMENT //////////////////////////
    sql  = (String) "SELECT COUNT (" + MESSAGE_SEQUENCE_ID + ")"
         +         " FROM "   + TABLE_GROUP_AND_PUBLISHER + ", " + MESSAGE_SEQUENCE_ID_TABLE
         +         " WHERE "  + GRP_NAME + " = ?1 "
         +         " AND "    + PUBLISHER_NODE_ID + " = ?2 "
         +         " AND "    + MESSAGE_SEQUENCE_ID + " = ?3 "
         +         " AND "    + TABLE_GROUP_AND_PUBLISHER + "." + GROUP_AND_PUBLISHER_ROW_ID + " = "
                              + MESSAGE_SEQUENCE_ID_TABLE + "." + GROUP_AND_PUBLISHER_ID +";";
    _psqlSelectGrpPubSeqCount = (*pDB)->prepare (sql.c_str());
    if (_psqlSelectGrpPubSeqCount == NULL) {
        _m.unlock();
        return -7;
    }

    sql  = (String) "SELECT " + GRP_NAME + ", " + PUBLISHER_NODE_ID + ", " + MESSAGE_SEQUENCE_ID
         +         " FROM "   + TABLE_GROUP_AND_PUBLISHER + ", " + MESSAGE_SEQUENCE_ID_TABLE
         +         " WHERE "  + GRP_NAME + " = ?1 "
         +         " AND "    + PUBLISHER_NODE_ID + " = ?2 "
         +         " AND "    + MESSAGE_SEQUENCE_ID + " = ?3 "
         +         " AND "    + TABLE_GROUP_AND_PUBLISHER + "." + GROUP_AND_PUBLISHER_ROW_ID + " = "
                              + MESSAGE_SEQUENCE_ID_TABLE + "." + GROUP_AND_PUBLISHER_ID +";";
    _psqlSelectGrpPubSeq = (*pDB)->prepare (sql.c_str());
    if (_psqlSelectGrpPubSeq == NULL) {
        _m.unlock();
        return -8;
    }

    sql  = (String) "SELECT MAX (" + MESSAGE_SEQUENCE_ID + ")"
         +         " FROM "   + TABLE_GROUP_AND_PUBLISHER + ", " + MESSAGE_SEQUENCE_ID_TABLE
         +         " WHERE "  + GRP_NAME + " = ?1 "
         +         " AND "    + PUBLISHER_NODE_ID + " = ?2 "
         +         " AND "    + TABLE_GROUP_AND_PUBLISHER + "." + GROUP_AND_PUBLISHER_ROW_ID + " = "
                              + MESSAGE_SEQUENCE_ID_TABLE + "." + GROUP_AND_PUBLISHER_ID +";";
    _psqlSelectGrpPubSeqMax = (*pDB)->prepare (sql.c_str());
    if (_psqlSelectGrpPubSeqMax == NULL) {
        _m.unlock();
        return -7;
    }

    sql  = (String) "SELECT " + GRP_NAME + ", " + PUBLISHER_NODE_ID + ", " + MESSAGE_SEQUENCE_ID
         +         " FROM "   + TABLE_GROUP_AND_PUBLISHER + ", " + MESSAGE_SEQUENCE_ID_TABLE
         +         " WHERE "  + TABLE_GROUP_AND_PUBLISHER + "." + GROUP_AND_PUBLISHER_ROW_ID + " = "
                              + MESSAGE_SEQUENCE_ID_TABLE + "." + GROUP_AND_PUBLISHER_ID + " "
         +         " ORDER BY " + GRP_NAME + ", " + PUBLISHER_NODE_ID + ";";
    _psqlSelectGrpPubSeqAll = (*pDB)->prepare (sql.c_str());
    if (_psqlSelectGrpPubSeqAll == NULL) {
        _m.unlock();
        return -9;
    }

    sql  = (String) "SELECT " + GROUP_AND_PUBLISHER_ROW_ID
         +         " FROM "   + TABLE_GROUP_AND_PUBLISHER
         +         " WHERE "  + GRP_NAME + " = ?1 "
         +         " AND "    + PUBLISHER_NODE_ID + " = ?2;";
    _psqlSelectGrpPubRowId = (*pDB)->prepare (sql.c_str());
    if (_psqlSelectGrpPubRowId == NULL) {
        _m.unlock();
        return -10;
    }

    _m.unlock();
    return 0;
}

int ReceivedMessages::addMessage (const char *pszGroupName, const char *pszPublisherNodeId,
                                  uint32 ui32MsgSeqId)
{
    _m.lock();

    // Check if the msg is already stored in ReceivedMessages.
    bool bContains;
    contains (pszGroupName, pszPublisherNodeId, ui32MsgSeqId, bContains);
    if (bContains) {
        // <grpName:senderId:msgSeqId> already in ReceivedMessages. An insert would cause a SQL constraint violation.
        _m.unlock();
        return -1;
    }

    // Check if the couple <grpName:senderId> already exists. If not, add it.
    int64 i64RowId = getGrpPubRowId (pszGroupName, pszPublisherNodeId);
    if (i64RowId <= 0) {
        if (!bind (pszGroupName, pszPublisherNodeId, _psqlInsertGrpPub)) {
            _m.unlock();
            return -2;
        }
        if (_psqlInsertGrpPub->update() < 0) {
            _m.unlock();
            return -3;
        }
        _psqlInsertGrpPub->reset();

        i64RowId = getGrpPubRowId (pszGroupName, pszPublisherNodeId);
        if (i64RowId < 0) {
            _m.unlock();
            return -4;
        }
    }

    // Insert msgSeqId, associated with the <grpName:senderId> id
    if (_psqlInsertMsgSeqId->bind ((unsigned int)1, ui32MsgSeqId) < 0 ||
        _psqlInsertMsgSeqId->bind ((unsigned int)2, i64RowId)) {
        _m.unlock();
        return -5;
    }
    if (_psqlInsertMsgSeqId->update() < 0) {
        _m.unlock();
        return -6;
    }
    _psqlInsertMsgSeqId->reset();

    _m.unlock();
    return 0;
}

int ReceivedMessages::contains (const char *pszGroupName, const char *pszPublisherNodeId,
                                uint32 ui32MsgSeqId, bool &bContains)
{
    bContains = false;
    _m.lock();
    if (!bind (pszGroupName, pszPublisherNodeId, ui32MsgSeqId, _psqlSelectGrpPubSeq)) {
        _m.unlock();
        return -1;
    }

    bContains = _psqlSelectGrpPubSeq->next (NULL);

    _psqlSelectGrpPubSeq->reset();
    _m.unlock();
    return 0;
}

int ReceivedMessages::getMaxSeqId (const char *pszGroupName, const char *pszPublisherNodeId,
                                   uint32 &ui32MaxMsgSeqId)
{
    ui32MaxMsgSeqId = 0;
    _m.lock();
    if (!bind (pszGroupName, pszPublisherNodeId, _psqlSelectGrpPubSeqMax)) {
        _m.unlock();
        return -1;
    }

    Row *pRow = _psqlSelectGrpPubSeqMax->getRow();
    if (pRow == NULL) {
        checkAndLogMsg ("ReceivedMessages::getMaxSeqId", memoryExhausted);
        _psqlSelectGrpPubSeqMax->reset();
        _m.unlock();
        return -2;
    }

    if (_psqlSelectGrpPubSeqMax->next (pRow)) {
        if (pRow->getValue (0, ui32MaxMsgSeqId) < 0) {
            ui32MaxMsgSeqId = 0;
            _psqlSelectGrpPubSeqMax->reset();
            _m.unlock();
            return -3;
        }
    }
    _psqlSelectGrpPubSeqMax->reset();
    _m.unlock();
    return 0;
}

StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> * ReceivedMessages::getReceivedMsgsByGrpPub (void)
{
    const char *pszMethodName = "ReceivedMessages::getReceivedMsgsByGrpPub";
    _m.lock();

    Row *pRow = _psqlSelectGrpPubSeqAll->getRow();
    if (pRow == NULL) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        _psqlSelectGrpPubSeqAll->reset();
        _m.unlock();
        return NULL;
    }

    StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pMsgs = NULL;
    while (_psqlSelectGrpPubSeqAll->next (pRow)) {
        if (pMsgs == NULL) {
            pMsgs = new StringHashtable<ReceivedMessages::ReceivedMsgsByGrp>();
            if (pMsgs == NULL) {
                checkAndLogMsg (pszMethodName, memoryExhausted);
                break;
            }
        }
        char *pszGroupName, *pszPublisherMsgId;
        uint32 ui32MsgSeqId;
        if (pRow->getValue ((unsigned short)0, &pszGroupName) == 0 &&
            pRow->getValue ((unsigned short)1, &pszPublisherMsgId) == 0 &&
            pRow->getValue ((unsigned short)2, ui32MsgSeqId) == 0) {

            ReceivedMsgsByGrp *pByGrp = pMsgs->get (pszGroupName);
            if (pByGrp == NULL) {
                pByGrp = new ReceivedMsgsByGrp;
                if (pByGrp != NULL) {
                    pByGrp->groupName = pszGroupName;
                    pMsgs->put (pszGroupName, pByGrp);
                }
                else {
                    checkAndLogMsg (pszMethodName, memoryExhausted);
                    break;
                }
            }

            ReceivedMsgsByPub *pByPub = pByGrp->msgsByPub.get (pszPublisherMsgId);
            if (pByPub == NULL) {
                pByPub = new ReceivedMsgsByPub;
                if (pByPub != NULL) {
                    pByPub->publisherNodeId = pszPublisherMsgId;
                    pByGrp->msgsByPub.put (pszPublisherMsgId, pByPub);
                }
                else {
                    checkAndLogMsg (pszMethodName, memoryExhausted);
                    break;
                }
            }

            if (pByPub != NULL) {
                pByPub->ranges.addTSN (ui32MsgSeqId);
            }
            else {
                break;
            }
        }
    }
    _psqlSelectGrpPubSeqAll->reset();
    _m.unlock();
    return pMsgs;
}

bool bind (const char *pszGroupName, const char *pszPublisherNodeId,
           uint32 ui32MsgSeqId, PreparedStatement *pStmt)
{
    pStmt->reset();
    if (!bind (pszGroupName, pszPublisherNodeId, pStmt) ||
        pStmt->bind ((unsigned short)3, ui32MsgSeqId) < 0) {
        checkAndLogMsg ("ReceivedMessages::bind",
                        Logger::L_SevereError, "Could not bind par\n");
        pStmt->reset();
        return false;
    }

    return true;
}

bool bind (const char *pszGroupName, const char *pszPublisherNodeId,
           PreparedStatement *pStmt)
{
    if (pszGroupName == NULL || pszPublisherNodeId == NULL) {
        return false;
    }

    pStmt->reset();
    if (pStmt->bind ((unsigned short)1, pszGroupName) < 0 ||
        pStmt->bind ((unsigned short)2, pszPublisherNodeId) < 0) {
        checkAndLogMsg ("ReceivedMessages::bind",
                        Logger::L_SevereError, "Could not bind par\n");
        pStmt->reset();
        return false;
    }
    return true;
}
