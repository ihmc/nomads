/*
 * SQLPropertyStore.cpp
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

#include "SQLPropertyStore.h"

#include "Database.h"
#include "PreparedStatement.h"
#include "Result.h"
#include "SessionId.h"

#include "Logger.h"

#include <stdlib.h>

using namespace IHMC_ACI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

const String SQLPropertyStore::TABLE_NAME = "NodeProperties";
const String SQLPropertyStore::NODE_ID_COL = "NodeId";
const String SQLPropertyStore::ATTR_COL = "Attribute";
const String SQLPropertyStore::VALUE_COL = "Value";

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace SQL_PROPERTY_STORE
{
    class ClearTable : public SessionIdListener
    {
        public:
            ClearTable (SQLPropertyStore *pProperties)
                : _pProperties (pProperties)
            {
            }

            ~ClearTable (void)
            {
            }

            void sessionIdChanged (void)
            {
                _pProperties->clear();
            }

        private:
            SQLPropertyStore *_pProperties;
    };
}

SQLPropertyStore::SQLPropertyStore (void)
{
    _ppsSetProperty = NULL;
    _ppsGetProperty = NULL;
    _ppsRemoveProperty = NULL;
}

SQLPropertyStore::~SQLPropertyStore (void)
{
    delete _ppsSetProperty;
    _ppsSetProperty = NULL;
    delete _ppsGetProperty;
    _ppsGetProperty = NULL;
    delete _ppsRemoveProperty;
    _ppsRemoveProperty = NULL;
}

int SQLPropertyStore::init (const char *pszStorageFile)
{
    int rc;

    _storageFile = pszStorageFile;

    // Initialize the DB
    DatabasePtr *pDB = Database::getDatabase (Database::SQLite);
    if (pDB == NULL) {
        checkAndLogMsg ("SQLPropertyStore::init", Logger::L_MildError,
                        "Database::getDatabase returned NULL\n");
        return -1;
    }
    if (0 != (rc = (*pDB)->open (_storageFile))) {
        checkAndLogMsg ("SQLPropertyStore::init", Logger::L_MildError,
                        "failed to open database; rc = %d\n", rc);
        return -2;
    }

    _m.lock();

    // Create the table if necessary
    String sql = (String) "CREATE TABLE IF NOT EXISTS " + TABLE_NAME +  " (" +
                          NODE_ID_COL + " TEXT NOT NULL, " +
                          ATTR_COL + " TEXT NOT NULL, " +
                          VALUE_COL + " TEXT NOT NULL, " +
                          "UNIQUE (" + NODE_ID_COL + ", " + ATTR_COL + "));";
    if (0 != (rc = (*pDB)->execute (sql))) {
        checkAndLogMsg ("SQLPropertyStore::init", Logger::L_MildError,
                        "failed to create table with statement [%s]; rc = %d\n",
                        (const char*) sql, rc);
        _m.unlock();
        return -3;
    }

    // Initialize the Prepared Statements
    sql = (String) "INSERT INTO " + TABLE_NAME + " VALUES (?1,?2,?3);";
    _ppsSetProperty = (*pDB)->prepare (sql);
    if (_ppsSetProperty == NULL) {
        checkAndLogMsg ("SQLPropertyStore::init", Logger::L_MildError,
                        "failed to prepare insert statement [%s]\n",
                        (const char*) sql);
        _m.unlock();
        return -4;
    }

    sql = (String) "SELECT " + VALUE_COL + " FROM " + TABLE_NAME +
                   " WHERE " + NODE_ID_COL + " = ?1 AND " + ATTR_COL + " = ?2;";
    _ppsGetProperty = (*pDB)->prepare (sql);
    if (_ppsGetProperty == NULL) {
        checkAndLogMsg ("SQLPropertyStore::init", Logger::L_MildError,
                        "failed to prepare select statement [%s]\n",
                        (const char*) sql);
        _m.unlock();
        return -5;
    }

    sql = (String) "DELETE FROM " + TABLE_NAME +
                   " WHERE " + NODE_ID_COL + " = ?1 AND " + ATTR_COL + " = ?2;";
    _ppsRemoveProperty = (*pDB)->prepare (sql);
    if (_ppsRemoveProperty == NULL) {
        checkAndLogMsg ("SQLPropertyStore::init", Logger::L_MildError,
                        "failed to prepare delete statement [%s]\n",
                        (const char*) sql);
        _m.unlock();
        return -6;
    }

    sql = (String) "UPDATE " + TABLE_NAME +
            " SET " + VALUE_COL + " = ?3 " +
            " WHERE " + NODE_ID_COL + " = ?1 AND " + ATTR_COL + " = ?2;";
    _ppsUpdateProperty = (*pDB)->prepare (sql);
    if (_ppsUpdateProperty == NULL) {
        checkAndLogMsg ("SQLPropertyStore::init", Logger::L_MildError,
                        "failed to prepare update statement [%s]\n",
                        (const char*) sql);
        _m.unlock();
        return -7;
    }

    sql = (String) "DELETE FROM " + TABLE_NAME + ";";
    _ppsDeleteAll = (*pDB)->prepare (sql);
    if (_ppsDeleteAll == NULL) {
        checkAndLogMsg ("SQLPropertyStore::init", Logger::L_MildError,
                        "failed to prepare delete statement [%s]\n",
                        (const char*)sql);
        _m.unlock();
        return -8;
    }

    _m.unlock();

    SessionId::getInstance()->registerSessionIdListener(new SQL_PROPERTY_STORE::ClearTable (this));

    return 0;
}

int SQLPropertyStore::set (const char *pszNodeID, const char *pszAttr, const char *pszValue)
{
    return setInternal (_ppsSetProperty, pszNodeID, pszAttr, pszValue);
}

String SQLPropertyStore::get (const char *pszNodeID, const char *pszAttr)
{
    if ((pszNodeID == NULL) || (pszAttr == NULL)) {
        return String ((const char*)NULL);
    }
    if ((strlen (pszNodeID) == 0) || (strlen (pszAttr) == 0)) {
        return String ((const char*)NULL);
    }
    _m.lock();
    _ppsGetProperty->reset();
    if (_ppsGetProperty->bind (1, pszNodeID) < 0) {
        _m.unlock();
        return String ((const char*)NULL);
    }
    if (_ppsGetProperty->bind (2, pszAttr) < 0) {
        _m.unlock();
        return String ((const char*)NULL);
    }

    Row *pRow = _ppsGetProperty->getRow();
    if (pRow == NULL) {
        _m.unlock();
        return String ((const char*)NULL);
    }

    char *pszValue = NULL;
    if (_ppsGetProperty->next (pRow)) {
        pRow->getValue (0, &pszValue);
    }
    String value (pszValue);
    if (pszValue) {
        free (pszValue);
    }
    delete pRow;
    _m.unlock();
    return value;
}

int SQLPropertyStore::remove (const char *pszNodeID, const char *pszAttr)
{
    if ((pszNodeID == NULL) || (pszAttr == NULL)) {
        return -1;
    }
    if ((strlen (pszNodeID) == 0) || (strlen (pszAttr) == 0)) {
        return -2;
    }
    _m.lock();
    _ppsRemoveProperty->reset();
    if (_ppsRemoveProperty->bind (1, pszNodeID) < 0) {
        _m.unlock();
        return -3;
    }
    if (_ppsRemoveProperty->bind (2, pszAttr) < 0) {
        _m.unlock();
        return -4;
    }
    if (_ppsRemoveProperty->update() < 0) {
        _m.unlock();
        return -5;
    }
    _m.unlock();
    return 0;
}

int SQLPropertyStore::update (const char *pszNodeID, const char *pszAttr, const char *pszNewValue)
{
    return setInternal (_ppsUpdateProperty, pszNodeID, pszAttr, pszNewValue);
}

void SQLPropertyStore::clear (void)
{
    const char *pszMethodName = "SQLPropertyStore::clear";
    const String query ("DELETE FROM DisServiceDataCache WHERE Attribute NOT LIKE 'dspro.latestMsgSeqId.DSPro%reset%' OR Attribute NOT LIKE 'latestResetMessage';");

    _m.lock();

    _ppsDeleteAll->reset();
    if (_ppsDeleteAll->update() == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "%s table cleared\n",
                        TABLE_NAME.c_str());
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not clear %s table\n",
                        TABLE_NAME.c_str());
    }

    _m.unlock();
}

int SQLPropertyStore::setInternal (PreparedStatement *pPreparedStmt, const char *pszNodeID, const char *pszAttr, const char *pszValue)
{
    if ((pPreparedStmt == NULL) || (pszNodeID == NULL) || (pszAttr == NULL) || (pszValue == NULL)) {
        return -1;
    }
    if ((strlen (pszNodeID) == 0) || (strlen (pszAttr) == 0) || (strlen (pszValue) == 0)) {
        return -2;
    }
    _m.lock();
    pPreparedStmt->reset();
    if (pPreparedStmt->bind (1, pszNodeID) < 0) {
        _m.unlock();
        return -3;
    }
    if (pPreparedStmt->bind (2, pszAttr) < 0) {
        _m.unlock();
        return -4;
    }
    if (pPreparedStmt->bind (3, pszValue) < 0) {
        _m.unlock();
        return -5;
    }
    if (pPreparedStmt->update() < 0) {
        _m.unlock();
        return -6;
    }
    _m.unlock();
    return 0;
}

