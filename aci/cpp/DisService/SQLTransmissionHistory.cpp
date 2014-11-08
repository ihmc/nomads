/*
 * SQLTransmissionHistory.cpp
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

#include "SQLTransmissionHistory.h"

#include "DisServiceDefs.h"
#include "DisServiceMsg.h"

#include "Database.h"
#include "PreparedStatement.h"
#include "Result.h"

#include "Logger.h"
#include "StringHashset.h"
#include "StringTokenizer.h"

#include "sqlite3.h"
#include "PtrLList.h"
#include "NLFLib.h"

#include <stddef.h>

using namespace IHMC_ACI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

const NOMADSUtil::String SQLTransmissionHistory::TABLE_MESSAGE_IDS = "MessageIDs";
const NOMADSUtil::String SQLTransmissionHistory::TABLE_TARGETS = "TargetIDs";
const NOMADSUtil::String SQLTransmissionHistory::TABLE_REL = "MessageTargets";
const NOMADSUtil::String SQLTransmissionHistory::MSG_ID = "MsgId";
const NOMADSUtil::String SQLTransmissionHistory::TARGET_ID = "TargetID";
const NOMADSUtil::String SQLTransmissionHistory::MSG_ROW_ID = "MsgRowId";
const NOMADSUtil::String SQLTransmissionHistory::TARGET_ROW_ID = "TargetRowID";

SQLTransmissionHistory::SQLTransmissionHistory (const char *pszStorageFile)
{
    _pszStorageFile = pszStorageFile;
    construct();
}

SQLTransmissionHistory::~SQLTransmissionHistory()
{
    _pszStorageFile = NULL;
    if (_psqlInsertMessage != NULL) {
        delete _psqlInsertMessage;
        _psqlInsertMessage = NULL;
    }
    if (_psqlInsertTargets != NULL) {
        delete _psqlInsertTargets;
        _psqlInsertTargets = NULL;
    }
    if (_psqlInsertMessageTargetsRel != NULL) {
        delete _psqlInsertMessageTargetsRel;
        _psqlInsertMessageTargetsRel = NULL;
    }
    if (_psqlSelectMessageRow != NULL) {
        delete _psqlSelectMessageRow;
        _psqlSelectMessageRow = NULL;
    }
    if (_psqlSelectTargetRow != NULL) {
        delete _psqlSelectTargetRow;
        _psqlSelectTargetRow = NULL;
    }
    if (_psqlSelectMessageTargets != NULL) {
        delete _psqlSelectMessageTargets;
        _psqlSelectMessageTargets = NULL;
    }
    if (_psqlSelectListMessageTargets != NULL) {
        delete _psqlSelectListMessageTargets;
        _psqlSelectListMessageTargets = NULL;
    }
    if (_psqlSelectListMessageByTarget != NULL) {
        delete _psqlSelectListMessageByTarget;
        _psqlSelectListMessageByTarget = NULL;
    }
    if (_psqlSelectCountMessage != NULL) {
        delete _psqlSelectCountMessage;
        _psqlSelectCountMessage = NULL;
    }
    if (_psqlSelectCountTargets != NULL) {
        delete _psqlSelectCountTargets;
        _psqlSelectCountTargets = NULL;
    }
    if (_psqlSelectAllMessages != NULL) {
        delete _psqlSelectAllMessages;
        _psqlSelectAllMessages = NULL;
    }

    if (_psqlResetRelations != NULL) {
        delete _psqlResetRelations;
        _psqlResetRelations = NULL;
    }
    if (_psqlResetTargets != NULL) {
        delete _psqlResetTargets;
        _psqlResetTargets = NULL;
    }
    if (_psqlResetMessages != NULL) {
        delete _psqlResetMessages;
        _psqlResetMessages = NULL;
    }

    if (_psqlDeleteExpRelations != NULL) {
        delete _psqlDeleteExpRelations;
        _psqlDeleteExpRelations = NULL;
    }
    if (_psqlDeleteExpMessages != NULL) {
        delete _psqlDeleteExpMessages;
        _psqlDeleteExpMessages = NULL;
    }
}

void SQLTransmissionHistory::construct()
{   
    _psqlInsertMessage = NULL;
    _psqlInsertTargets = NULL;
    _psqlInsertMessageTargetsRel = NULL;
    _psqlSelectMessageRow = NULL;
    _psqlSelectTargetRow = NULL;
    _psqlSelectMessageTargets = NULL;
    _psqlSelectListMessageTargets = NULL;
    _psqlSelectListMessageByTarget = NULL;
    _psqlSelectCountMessage = NULL;
    _psqlSelectCountTargets = NULL;
    _psqlSelectAllMessages = NULL;

    _psqlResetRelations = NULL;
    _psqlResetTargets = NULL;
    _psqlResetMessages = NULL;

    _psqlDeleteExpRelations = NULL;
    _psqlDeleteExpMessages = NULL;
}

int SQLTransmissionHistory::init()
{
    const char *pszMethodName = "SQLTransmissionHistory::init";
    Database *pDB = Database::getDatabase (Database::SQLite);
    if (pDB == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Database::getDatabase returned NULL pointer.\n");
        return -1;
    }
    if (pDB->open (_pszStorageFile) != 0) {
        return -2;
    }

    _m.lock();

    //////////////////////// CREATE TABLE MessageIDs //////////////////////////////
    // In SQLite, every row of every table has an 64-bit signed integer ROWID.
    // The ROWID for each row is unique among all rows in the same table.
    // If a table contains a column of type INTEGER PRIMARY KEY, then that column
    // becomes an alias for the ROWID.
    // Therefore MSG_ROW_ID is an alias for ROWID
    
    String sql = (String) "CREATE TABLE IF NOT EXISTS " + TABLE_MESSAGE_IDS +  "("
               +           MSG_ROW_ID + " INTEGER PRIMARY KEY, "
               +           MSG_ID + " TEXT, UNIQUE (" + MSG_ID + "));";
    if (pDB->execute (sql.c_str()) != 0) {
        _m.unlock();
        return -3;
    }

    //////////////////////// CREATE TABLE TargetIDs ////////////////////////////
    // In SQLite, every row of every table has an 64-bit signed integer ROWID.
    // The ROWID for each row is unique among all rows in the same table.
    // If a table contains a column of type INTEGER PRIMARY KEY, then that column
    // becomes an alias for the ROWID.
    // Therefore TARGET_ROW_ID is an alias for ROWID
    sql = (String) "CREATE TABLE IF NOT EXISTS " + TABLE_TARGETS +  " ("
        +           TARGET_ROW_ID + " INTEGER PRIMARY KEY, "
        +           TARGET_ID + " TEXT, UNIQUE (" + TARGET_ID + "));";
    if (pDB->execute (sql.c_str()) != 0) {
        _m.unlock();
        return -4;
    }

    //////////////////////// CREATE TABLE Join //////////////////////////////
    sql = (String) "CREATE TABLE IF NOT EXISTS " + TABLE_REL +  " ("
        +          MSG_ROW_ID + " INT, "
        +          TARGET_ROW_ID + " INT, "
        +          "FOREIGN KEY (" + MSG_ROW_ID + ") REFERENCES " + TABLE_MESSAGE_IDS + "(MSG_ROW_ID) "
        +          "FOREIGN KEY (" + TARGET_ROW_ID + ") REFERENCES " + TABLE_TARGETS + "(TARGET_ROW_ID));";
    if (pDB->execute (sql.c_str()) != 0) {
        _m.unlock();
        return -5;
    }

    ///////////////////////// CREATE SELECT STATEMENT //////////////////////////
    sql  = (String) "SELECT " + TABLE_TARGETS + "." + TARGET_ID
         +         " FROM " + TABLE_MESSAGE_IDS + ", " + TABLE_REL + ", " + TABLE_TARGETS
         +         " WHERE " + TABLE_MESSAGE_IDS + "." + MSG_ID + " = ?1 "
         +         " AND " + TABLE_TARGETS + "." + TARGET_ROW_ID + " = " + TABLE_REL + "." + TARGET_ROW_ID
         +         " AND " + TABLE_MESSAGE_IDS + "." + MSG_ROW_ID + " = " + TABLE_REL + "." + MSG_ROW_ID +";";
    _psqlSelectListMessageTargets = pDB->prepare (sql.c_str());
    if (_psqlSelectListMessageTargets == NULL) {
        _m.unlock();
        return -6;
    }

    ///////////////////////// CREATE SELECT STATEMENT //////////////////////////
    sql  = (String) "SELECT " + TABLE_MESSAGE_IDS + "." + MSG_ID
         +         " FROM " + TABLE_MESSAGE_IDS + ", " + TABLE_REL + ", " + TABLE_TARGETS
         +         " WHERE " + TABLE_TARGETS + "." + TARGET_ID + " = ?1 "
         +         " AND " + TABLE_TARGETS + "." + TARGET_ROW_ID + " = " + TABLE_REL + "." + TARGET_ROW_ID
         +         " AND " + TABLE_MESSAGE_IDS + "." + MSG_ROW_ID + " = " + TABLE_REL + "." + MSG_ROW_ID +";";
    _psqlSelectListMessageByTarget = pDB->prepare (sql.c_str());
    if (_psqlSelectListMessageByTarget == NULL) {
        _m.unlock();
        return -7;
    }

    ///////////////////////// CREATE SELECT STATEMENT //////////////////////////
    sql  = (String) "SELECT " + TABLE_MESSAGE_IDS + "." + MSG_ROW_ID;
    sql = sql + " FROM " + TABLE_MESSAGE_IDS;
    sql = sql + " WHERE " + TABLE_MESSAGE_IDS + "." + MSG_ID + " = ?1;";
    _psqlSelectMessageRow = pDB->prepare (sql.c_str());
    if (_psqlSelectMessageRow == NULL) {
        _m.unlock();
        return -8;
    }

    ///////////////////////// CREATE SELECT STATEMENT //////////////////////////
    sql  = (String) "SELECT " + TABLE_MESSAGE_IDS + "." + MSG_ID;
    sql = sql + " FROM " + TABLE_MESSAGE_IDS + ";";
    _psqlSelectAllMessages = pDB->prepare (sql.c_str());
    if (_psqlSelectAllMessages == NULL) {
        _m.unlock();
        return -8;
    }

    ///////////////////////// CREATE SELECT STATEMENT //////////////////////////
    sql  = (String) "SELECT " + TABLE_TARGETS + "." + TARGET_ROW_ID;
    sql = sql + " FROM " + TABLE_TARGETS;
    sql = sql + " WHERE " + TABLE_TARGETS + "." + TARGET_ID + " = ?;";
    _psqlSelectTargetRow = pDB->prepare (sql.c_str());
    if (_psqlSelectTargetRow == NULL) {
        _m.unlock();
        return -9;
    }

    ///////////////////////// CREATE SELECT STATEMENT //////////////////////////
    sql  = (String) "SELECT " + TABLE_TARGETS + "." + TARGET_ID;
    sql = sql + " FROM " + TABLE_MESSAGE_IDS + ", " + TABLE_REL + ", " + TABLE_TARGETS;
    sql = sql + " WHERE " + TABLE_MESSAGE_IDS + "." + MSG_ID + " = ?1 ";
    sql = sql + " AND " + TABLE_TARGETS + "." + TARGET_ID + " = ?2 ";
    sql = sql + " AND " + TABLE_TARGETS + "." + TARGET_ROW_ID + " = " + TABLE_REL + "." + TARGET_ROW_ID;
    sql = sql + " AND " + TABLE_MESSAGE_IDS + "." + MSG_ROW_ID + " = " + TABLE_REL + "." + MSG_ROW_ID +";";
    _psqlSelectMessageTargets = pDB->prepare (sql.c_str());
    if (_psqlSelectMessageTargets == NULL) {
        _m.unlock();
        return -10;
    }

    ///////////////////////// CREATE SELECT COUNT STATEMENT ////////////////////
    sql  = (String) "SELECT COUNT (*) FROM " + TABLE_MESSAGE_IDS + ";";
    _psqlSelectCountMessage = pDB->prepare (sql.c_str());
    if (_psqlSelectCountMessage == NULL) {
        _m.unlock();
        return -11;
    }

    ///////////////////////// CREATE SELECT COUNT STATEMENT ////////////////////
    sql  = (String) "SELECT COUNT (*) FROM " + TABLE_TARGETS + ";";
    _psqlSelectCountTargets = pDB->prepare (sql.c_str());
    if (_psqlSelectCountTargets == NULL) {
        _m.unlock();
        return -12;
    }

    ///////////////////////// CREATE INSERT STATEMENT //////////////////////////
    sql = (String) "INSERT INTO " + TABLE_MESSAGE_IDS
        +          " (" + MSG_ID + ") VALUES (?);";
    _psqlInsertMessage = pDB->prepare (sql.c_str());
    if (_psqlInsertMessage == NULL) {
        _m.unlock();
        return -13;
    }

    ///////////////////////// CREATE INSERT STATEMENT //////////////////////////
    sql = (String) "INSERT INTO " + TABLE_TARGETS
        +          " (" + TARGET_ID + ") VALUES (?);";
    _psqlInsertTargets = pDB->prepare (sql.c_str());
    if (_psqlInsertTargets == NULL) {
        _m.unlock();
        return -14;
    }

    ///////////////////////// CREATE INSERT STATEMENT //////////////////////////
    sql = (String) "INSERT INTO " + TABLE_REL
        +          " VALUES (?,?);";
    _psqlInsertMessageTargetsRel = pDB->prepare (sql.c_str());
    if (_psqlInsertMessageTargetsRel == NULL) {
        _m.unlock();
        return -15;
    }

    ///////////////////// CREATE RESET RELATIONS STATEMENT /////////////////////

    sql = (String) "DELETE FROM " + TABLE_REL + ";";       
    _psqlResetRelations = pDB->prepare (sql.c_str());
    if (_psqlResetRelations == NULL) {
        _m.unlock();
        return -16;
    }

    ///////////////////// CREATE RESET MESSAGES STATEMENT //////////////////////

    sql = (String) "DELETE FROM " + TABLE_MESSAGE_IDS + ";";
    _psqlResetMessages = pDB->prepare (sql.c_str());
    if (_psqlResetMessages == NULL) {
        _m.unlock();
        return -17;
    }

    ////////////////////// CREATE RESET TARGETS STATEMENT //////////////////////

    sql = (String) "DELETE FROM " + TABLE_TARGETS + ";";
    _psqlResetTargets = pDB->prepare (sql.c_str());
    if (_psqlResetTargets == NULL) {
        _m.unlock();
        return -18;
    }

    ////////// CREATE DELETE RELATIONS OF EXPIRED MESSAGES STATEMENT ///////////

    sql = (String) "DELETE FROM " + TABLE_REL
        + " WHERE " + TABLE_REL + "." + MSG_ROW_ID + " = ?;" ;
    _psqlDeleteExpRelations = pDB->prepare (sql.c_str());
    if (_psqlDeleteExpRelations == NULL) {
        _m.unlock();
        return -19;
    }

    ///////////////// CREATE DELETE EXPIRED MESSAGES STATEMENT /////////////////

    sql = (String) "DELETE FROM " + TABLE_MESSAGE_IDS
        + " WHERE " + TABLE_MESSAGE_IDS + "." + MSG_ROW_ID + " = ?;" ;
    _psqlDeleteExpMessages = pDB->prepare (sql.c_str());
    if (_psqlDeleteExpMessages == NULL) {
        _m.unlock();
        return -20;
    }

    _m.unlock();
    return 0;
}

void SQLTransmissionHistory::messageSent (DisServiceMsg *pDisServiceMsg)
{
    if (pDisServiceMsg == NULL) {
        return;
    }
    if (pDisServiceMsg->getType() == DisServiceMsg::DSMT_Data) {

    }
    else if (pDisServiceMsg->getType() == DisServiceMsg::DSMT_AcknowledgmentMessage) {

    }
}

int SQLTransmissionHistory::addMessageRecord (const char *pszKey)
{
    if (_psqlInsertMessage->bind (1, pszKey) < 0) {
        return -1;
    }
    int rc = _psqlInsertMessage->update();
    if (rc > 0) {
        rc = 0;
    }
    _psqlInsertMessage->reset();
    return rc;
}

int SQLTransmissionHistory::addTargetRecord (const char *pszNewRecipientList)
{
    if (_psqlInsertTargets->bind (1, pszNewRecipientList) < 0) {
        return -1;
    }
    int rc = _psqlInsertTargets->update();
    if (rc > 0) {
        rc = 0;
    }
    _psqlInsertTargets->reset();
    return rc;
}

int SQLTransmissionHistory::addMessageTargetRelRecord (int messageRowid, int targetRowid)
{
    if (_psqlInsertMessageTargetsRel->bind (1, messageRowid) < 0 ||
        _psqlInsertMessageTargetsRel->bind (2, targetRowid) < 0) {
        return -1;
    }
    int rc = _psqlInsertMessageTargetsRel->update();
    if (rc > 0) {
        rc = 0;
    }
    _psqlInsertMessageTargetsRel->reset();
    return rc;
}

int SQLTransmissionHistory::addMessageTarget (const char *pszKey, const char *pszTarget)
{
    _m.lock();
    if (pszKey == NULL || pszTarget == NULL) {
        _m.unlock();
        return -1;
    }

    Record *pMsgRec = getMessageRow (pszKey);
    if (pMsgRec == NULL) {
        int rc = addMessageRecord (pszKey);
        if (rc < 0 || (pMsgRec = getMessageRow (pszKey)) == NULL) {
            checkAndLogMsg ("SQLTransmissionHistory::addMessageTarget", Logger::L_Warning,
                            "could not add message id %s\n", pszKey);
            _m.unlock();
            return -1;
        }
    }

    Record *pTgtRec = getTargetRow (pszTarget);
    if (pTgtRec == NULL) {
        int rc = addTargetRecord (pszTarget);
        if (rc < 0 || (pTgtRec = getTargetRow (pszTarget)) == NULL) {
            checkAndLogMsg ("SQLTransmissionHistory::addMessageTarget", Logger::L_Warning,
                            "could not add target %s\n", pszTarget);
            delete pMsgRec;
            pMsgRec = NULL;
            _m.unlock();
            return -1;
        }
    }

    // Add a record to JOIN table
    int rc = addMessageTargetRelRecord (pMsgRec->rowidForeignKey, pTgtRec->rowidForeignKey);
    delete pMsgRec;
    delete pTgtRec;
    pMsgRec = NULL;
    pTgtRec = NULL;

    if (rc < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::addMessageTarget", Logger::L_Warning,
                        "could not add message id - target relationship between %s and %s\n",
                        pszKey, pszTarget);
        _m.unlock();
        return -1;
    }
    else {
        checkAndLogMsg ("SQLTransmissionHistory::addMessageTarget",
                        Logger::L_LowDetailDebug, "Inserted (%s, %s)\n",
                        pszKey, pszTarget);
        _m.unlock();
        return 0;
    }
}

const char ** SQLTransmissionHistory::getTargetList (const char *pszKey)
{
    _m.lock();
    const char **ppszList = getList (_psqlSelectListMessageTargets, pszKey);
    _m.unlock();
    return ppszList;
}

const char ** SQLTransmissionHistory::getMessageList (const char *pszTarget)
{
    if (pszTarget == NULL) {
        return NULL;
    }
    _m.lock();
    const char **ppszList = getList (_psqlSelectListMessageByTarget, pszTarget);
    _m.unlock();
    return ppszList;
}

const char ** SQLTransmissionHistory::getMessageList (const char *pszTarget, const char *pszForwarder)
{
    if (pszTarget == NULL || pszForwarder == NULL) {
        if (pszTarget != NULL) {
            return getMessageList (pszTarget);
        }
        return getMessageList (pszForwarder);
    }
    if (strcmp (pszTarget, pszForwarder) == 0) {
        return getMessageList (pszTarget);
    }

    StringHashset ids (true,  // bCaseSensitiveKeys
                       false, // bCloneKeys
                       false  // bDeleteKeys
    );

    _m.lock();
    const char **ppszList = getList (_psqlSelectListMessageByTarget, pszTarget);
    if (ppszList != NULL) {
        for (unsigned int i = 0; ppszList[i] != NULL; i++) {
            if (ids.containsKey (ppszList[i])) {
                free ((char *)ppszList[i]);
                ppszList[i] = NULL;
            }
            else {
                ids.put (ppszList[i]);
            }
        }
        free ((char **)ppszList);
        ppszList = NULL;
    }

    ppszList = getList (_psqlSelectListMessageByTarget, pszForwarder);
    if (ppszList != NULL) {
        for (unsigned int i = 0; ppszList[i] != NULL; i++) {
            if (ids.containsKey (ppszList[i])) {
                free ((char *)ppszList[i]);
                ppszList[i] = NULL;
            }
            else {
                ids.put (ppszList[i]);
            }
        }
        free ((char **)ppszList);
        ppszList = NULL;
    }
    _m.unlock();

    unsigned short uiNElements = ids.getCount();
    char ** ppszMergeList = (char **) calloc (uiNElements+1, sizeof (char*));
    if (ppszMergeList == NULL) {
        checkAndLogMsg ("SQLTransmissionHistory::getList (2)", memoryExhausted);
        return NULL;
    }

    StringHashset::Iterator it = ids.getAllElements();
    for (unsigned short i = 0; !it.end() && i < uiNElements; i++) {
        ppszMergeList[i] = (char*) it.getKey();
        it.nextElement();
    }
    ppszMergeList[uiNElements] = NULL;

    return (const char **) ppszMergeList;
}

const char ** SQLTransmissionHistory::getList (PreparedStatement *pStmt, const char *pszID)
{
    if (pszID == NULL) {
        return NULL;
    }
    if (pStmt == NULL) {
        checkAndLogMsg ("SQLTransmissionHistory::getList", Logger::L_SevereError,
                        "NULL prepared statement\n");
        return NULL;
    }

    if (pStmt->bind (1, pszID) < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::getList",
                        Logger::L_SevereError, "Could not bind par 1.\n");
        return NULL;
    }

    Row *pRow = pStmt->getRow();
    PtrLList<char> resultRows;    // PtrLList's destructor does not de-allocate
                                  // the nodes's elements, thus _resultRows
                                  // can be safely deallocate and the elements
                                  // of ppszList will not be deallocated
    unsigned int uiCount = 0;   
    for (char *pszTmp; pStmt->next (pRow); ) {
        if (pRow->getValue (0, &pszTmp) == 0) { // pRow->getValue returns a copy 
            resultRows.append (pszTmp);         // of the value
            uiCount++;
        }
    }
    pStmt->reset();

    if (uiCount == 0) {
        delete pRow;
        return NULL;
    }

    char **ppszList = (char **) calloc (uiCount + 1, (sizeof(char*)));
    if (ppszList == NULL) {
        checkAndLogMsg ("SQLTransmissionHistory::getList", memoryExhausted);
    }
    else {
        ppszList[0] = resultRows.getFirst();
        for (unsigned int i = 1 ; i < uiCount; i++) {
            ppszList[i] = resultRows.getNext();
        }
    }

    delete pRow;
    return (const char**)ppszList;
}

const char ** SQLTransmissionHistory::getAllMessageIds()
{
    _m.lock();
    Row *pRow = _psqlSelectAllMessages->getRow();
    if (pRow == NULL) {
        checkAndLogMsg ("SQLTransmissionHistory::getAllMessageIds", memoryExhausted);
        _m.unlock();
        return NULL;
    }
    PtrLList<char> resultRows;    // PtrLList's destructor does not de-allocate
                                  // the nodes's elements, thus _resultRows
                                  // can be safely deallocate and the elements
                                  // of ppszList will not be deallocated
    unsigned int uiCount = 0;   
    for (char *pszTmp; _psqlSelectAllMessages->next (pRow); ) {
        if (pRow->getValue (0, &pszTmp) == 0) { // pRow->getValue returns a copy 
            resultRows.append (pszTmp);         // of the value
            uiCount++;
        }
    }
    _psqlSelectAllMessages->reset();

    if (uiCount == 0) {
        delete pRow;
        _m.unlock();
        return NULL;
    }

    char **ppszList = (char **) calloc (uiCount + 1, (sizeof(char*)));
    if (ppszList == NULL) {
        checkAndLogMsg ("SQLTransmissionHistory::getAllMessageIds", memoryExhausted);
    }
    else {
        ppszList[0] = resultRows.getFirst();
        for (unsigned int i = 1 ; i < uiCount; i++) {
            ppszList[i] = resultRows.getNext();
        }
    }

    delete pRow;
    _m.unlock();
    return (const char**)ppszList;
}

void SQLTransmissionHistory::releaseList (const char **ppszTargetList)
{
    if (ppszTargetList != NULL) {
        for (int i = 0; ppszTargetList[i] != NULL; i++) {
            free ((char *)ppszTargetList[i]);
            ppszTargetList[i] = NULL;
        }
        free (ppszTargetList);
        ppszTargetList = NULL;
    }
}

bool SQLTransmissionHistory::hasMessage (const char *pszKey)
{
    _m.lock();
    if (pszKey == NULL) {
        _m.unlock();
        return false;
    }

    _psqlSelectListMessageTargets->reset();
    if (_psqlSelectListMessageTargets->bind (1, pszKey) < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::hasMessage",
                        Logger::L_SevereError, "Could not bind par\n");
        _m.unlock();
        return false;
    }

    Row *pRow = _psqlSelectListMessageTargets->getRow();
    bool bFound = _psqlSelectListMessageTargets->next (pRow);
    _psqlSelectListMessageTargets->reset();
    delete pRow;

    _m.unlock();
    return bFound;
}

SQLTransmissionHistory::Record * SQLTransmissionHistory::getMessageRow (const char *pszKey)
{
    if (pszKey == NULL) {
        return NULL;
    }

    _psqlSelectMessageRow->reset();
    if (_psqlSelectMessageRow->bind (1, pszKey) < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::getMessageRow",
                        Logger::L_SevereError, "Could not bind par\n");
        return NULL;
    }

    Row *pRow = _psqlSelectMessageRow->getRow();
    if (_psqlSelectMessageRow->next (pRow)) {
        // SQLITE_ROW is returned each time a new row of data is ready for
        // processing by the caller, therefore the target is included in the
        // target list for the message identified by pszKey
        Record *pRecord = new Record;
        if (pRecord == NULL) {
            checkAndLogMsg ("SQLTransmissionHistory::getMessageRow", memoryExhausted);
        }
        else {
            pRow->getValue (0, pRecord->rowidForeignKey);
        }
        _psqlSelectMessageRow->reset();
        delete pRow;
        return pRecord;
    }

    delete pRow;
    _psqlSelectMessageRow->reset();
    return NULL;
}

SQLTransmissionHistory::Record * SQLTransmissionHistory::getTargetRow (const char *pszTarget)
{
    if (pszTarget == NULL) {
        return NULL;
    }

    _psqlSelectTargetRow->reset();
    if (_psqlSelectTargetRow->bind (1, pszTarget) < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::getTargetRow",
                        Logger::L_SevereError, "Could not bind par 1.\n");
        return NULL;
    }

    Row *pRow = _psqlSelectTargetRow->getRow();
    if (_psqlSelectTargetRow->next (pRow)) {
        // SQLITE_ROW is returned each time a new row of data is ready for
        // processing by the caller, therefore the target is included in the
        // target list for the message identified by pszKey
        Record *pRecord = new Record;
        if (pRecord == NULL) {
            checkAndLogMsg ("SQLTransmissionHistory::getTargetRow", memoryExhausted);
        }
        else {
            pRow->getValue (0, pRecord->rowidForeignKey);
        }
        delete pRow;
        _psqlSelectTargetRow->reset();
        return pRecord;
    }

    delete pRow;
    _psqlSelectTargetRow->reset();
    return NULL;
}

bool SQLTransmissionHistory::hasTarget (const char *pszKey, const char *pszTarget)
{
    _m.lock();
    if (pszTarget == NULL) {
        _m.unlock();
        return false;
    }

    _psqlSelectMessageTargets->reset();
    if (_psqlSelectMessageTargets->bind (1, pszKey) < 0 ||
        _psqlSelectMessageTargets->bind (2, pszTarget) < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::hasTarget",
                        Logger::L_SevereError, "Could not bind par\n");
        _psqlSelectMessageTargets->reset();
        _m.unlock();
        return false;
    }

    bool bHasTarget = _psqlSelectMessageTargets->next (NULL);
    _psqlSelectMessageTargets->reset();
    _m.unlock();
    return bHasTarget;
}

bool SQLTransmissionHistory::hasTargets (const char *pszKey, const char **ppszTargets)
{
    const char *pszMethodName = "SQLTransmissionHistory::hasTarget";
    _m.lock();
    if (ppszTargets == NULL) {
        _m.unlock();
        return false;
    }

    Database *pDB = Database::getDatabase (Database::SQLite);
    if (pDB == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Database::getDatabase returned NULL pointer.\n");
        _m.unlock();
        return false;
    }
    if (pDB->open (_pszStorageFile) != 0) {
        _m.unlock();
        return false;
    }
 
    String sql = (String) "SELECT COUNT (*)";
    sql = sql + " FROM " + TABLE_MESSAGE_IDS + ", " + TABLE_REL + ", " + TABLE_TARGETS;
    sql = sql + " WHERE " + TABLE_TARGETS + ".rowid = " + TABLE_REL + "." + TARGET_ID;
    sql = sql + " AND " + TABLE_MESSAGE_IDS + "." + MSG_ID + " = " + TABLE_REL + "." + MSG_ID;
    sql = sql + " AND " + TABLE_MESSAGE_IDS + "." + MSG_ID + " = '" + pszKey + "' AND (";

    int i = 0;
    for (const char *pszTarget = ppszTargets[0]; pszTarget != NULL; pszTarget = ppszTargets[i]) {
        if(i!=0) {
            sql = sql + " OR ";
        }
        sql = sql + TABLE_TARGETS + "." + TARGET_ID + " = '" + pszTarget + "'";
        i++;
    }
    sql = sql +");";

    PreparedStatement *psqlSelectAllTargets = pDB->prepare (sql.c_str());
    if (psqlSelectAllTargets == NULL) {
        checkAndLogMsg ("SQLTransmissionHistory::hasTargets ", Logger::L_SevereError,
                        "Could not prepare statement,\n");
        _m.unlock();
        return false;
    }

    int nFoundElem;
    Row *pRow = psqlSelectAllTargets->getRow();
    bool bRet = false;
    if (psqlSelectAllTargets->next (pRow) && pRow->getValue (0, nFoundElem) >= 0) {
        bRet = true;
    }
    delete pRow;
    psqlSelectAllTargets->reset();

    _m.unlock();
    return bRet;
}

int SQLTransmissionHistory::reset()
{
    _m.lock();

    if (_psqlResetRelations == NULL) {
        _m.unlock();
        return -1;
    }
    if (_psqlResetRelations->update() < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::reset ",
                        Logger::L_SevereError, "Reset-relations query failed\n");
        // If this delete fails, I can not proceed, because if the next
        // deletes went through there would be foreign key constraint
        // violations
        _m.unlock();
        return -2;
    }

    if (_psqlResetTargets != NULL &&
        _psqlResetTargets->update() < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::reset ",
                        Logger::L_Warning, "Reset-targets query failed\n");
    }

    if (_psqlResetMessages != NULL &&
        _psqlResetMessages->update() < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::reset ",
                        Logger::L_Warning, "Reset-relations query failed\n");
    }

    _m.unlock();
    return 0;
}

int SQLTransmissionHistory::deleteMessage (const char *pszMsgId)
{
    _m.lock();

    if (pszMsgId == NULL) {
        _m.unlock();
        return -1;
    }

    // Get the MSG_ROW_ID of the message to delete
    Record *pMsgRec = getMessageRow (pszMsgId);
    if (pMsgRec == NULL) {
        checkAndLogMsg ("SQLTransmissionHistory::getMessageRow",
                        Logger::L_SevereError, "Message to delete not found\n");
        _m.unlock();
        return -2;
    }
    int iRowId = pMsgRec->rowidForeignKey;
    delete pMsgRec;

    // Bind the parameter msg row id to delete exp rel statement
    if (_psqlDeleteExpRelations == NULL) {
        _m.unlock();
        return -3;
    }
    _psqlDeleteExpRelations->reset();
    if (_psqlDeleteExpRelations->bind (1, iRowId) < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::deleteMessage",
                        Logger::L_SevereError, "Could not bind par rel\n");
        _m.unlock();
        return -4;
    }

    // Delete messages from relations table
    if (_psqlDeleteExpRelations->update() < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::deleteMessage ",
                        Logger::L_SevereError, "Delete-exp-relation query failed\n");
        // If this delete fails, I can not proceed, because if the next
        // delete went through there would be foreign key constraint
        // violations
        _m.unlock();
        return -5;
    }

    // Bind the parameter msg row id to delete exp msgs statement
    if (_psqlDeleteExpMessages == NULL) {
        _m.unlock();
        return -6;
    }
    _psqlDeleteExpMessages->reset();
    if (_psqlDeleteExpMessages->bind (1, iRowId) < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::deleteMessage",
                        Logger::L_SevereError, "Could not bind par msgs\n");
        _m.unlock();
        return -7;
    }

    // Delete messages from messages table
    if (_psqlDeleteExpMessages != NULL &&
        _psqlDeleteExpMessages->update() < 0) {
        checkAndLogMsg ("SQLTransmissionHistory::deleteMessage ",
                        Logger::L_Warning, "Delete-exp-message query failed\n");
    }

    _m.unlock();
    return 0;
}
