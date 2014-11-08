/*
  * Database.cpp
  *
  * This file is part of the IHMC Misc Library
  * Copyright (c) 2011-2014 IHMC.
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

#include "Database.h"

#include "LCppDCDefs.h"
#include "PreparedStatement.h"
#include "Result.h"
#include "SQLiteFactory.h"

#include "Logger.h"

#include "sqlite3.h"

#include <stdlib.h>
#include <string.h>

using namespace IHMC_MISC;
using namespace NOMADSUtil;

//=============================================================================
// Database
//=============================================================================

Database * Database::_pDB = NULL;

Database::Database (Type type)
    : _type (type)
{
}

Database::~Database()
{
    _pDB = NULL;
}

Database * Database::getDatabase (Type type)
{
    if (_pDB != NULL) {
        checkAndLogMsg ("Database::getDatabase", Logger::L_SevereError,
                        "a database of type %s was requested when a database of "
                        "type %s had already been instantiated\n",
                        getTypeAsString (type), getTypeAsString (_pDB->getType()));
        return _pDB;
    }

    switch (type) {
        case SQLite:
            _pDB = new SQLiteDatabase();
            break;

        default:
            checkAndLogMsg ("Database::getDatabase", Logger::L_SevereError,
                            "unknown database type: %s\n", getTypeAsString (type));
            return NULL;		
    }

    return _pDB;
}

Database::Type Database::getType (void) const
{
    return _type;
}

const char * Database::getDBName (void) const
{
    return _dbName.c_str(); 
}

const char * Database::getTypeAsString (Type type)
{
    switch (type) {
        case SQLite:
            return "SQLite";
    }
    return NULL;
}

//=============================================================================
// SQLiteDatabase
//=============================================================================

const char * SQLiteDatabase::IN_MEMORY = ":memory:";

const char * SQLiteDatabase::SQLITE_PRAGMAS[] = {
    "cache_size",
    "journal_mode ",
    "journal_size_limit",
    "locking_mode",
    "synchronous"  
};

SQLiteDatabase::SQLiteDatabase()
    : Database (Database::SQLite)
{
    _pDB = NULL;
}

SQLiteDatabase::~SQLiteDatabase()
{
    close();
}

int SQLiteDatabase::open (const char *pszDBName)
{
    if (pszDBName == NULL) {
        pszDBName = IN_MEMORY;
    }
    if ((_pDB = SQLiteFactory::getInstance (pszDBName)) == NULL) {
        checkAndLogMsg ("SQLiteDatabase::open", Logger::L_SevereError,
                        "could not open database %s open\n", pszDBName);
        return -1;
    }
    checkAndLogMsg ("SQLiteDatabase::open", Logger::L_Info,
                    "database %s successfully open - using SQLite version %s\n",
                    pszDBName, SQLITE_VERSION);
    _dbName = pszDBName;
    return 0;
}

int SQLiteDatabase::close()
{
    if (_pDB != NULL) {
        SQLiteFactory::close();
        _pDB = NULL;
    }
    return 0;
}

int SQLiteDatabase::execute (const char *pszStatement)
{
    if (pszStatement == NULL) {
        checkAndLogMsg ("SQLiteDatabase::prepare", emptySqlStmt);
        return -1; 
    }

    char *pszErrMsg = NULL;
    int rc = sqlite3_exec (_pDB, pszStatement, NULL, NULL, &pszErrMsg);
    switch (rc) {
        case SQLITE_OK:
            checkAndLogMsg ("SQLiteDatabase::execute", successfulSqlStmt, pszStatement);
            break;

        default:
            checkAndLogMsg ("SQLiteDatabase::execute", failureSqlStmt,
                            SQLiteFactory::getErrorAsString (rc), pszStatement,
                            (pszErrMsg != NULL ? pszErrMsg : ""));
            sqlite3_free (pszErrMsg);
            return -3;
    }
    return 0;
}

int SQLiteDatabase::execute (const char *pszStatement, Table *pTable)
{
    return executeInternal (pszStatement, (AbstractSQLiteTable *) pTable);
}

int SQLiteDatabase::executeInternal (const char *pszStatement, AbstractSQLiteTable *pTable)
{
    if (pszStatement == NULL) {
        checkAndLogMsg ("SQLiteDatabase::prepare", emptySqlStmt);
        return -1; 
    }

    char **ppszResult;
    int iRow = 0;
    int iCol = 0;
    char *pszErrMsg = NULL;
    int rc = sqlite3_get_table (_pDB, pszStatement, &ppszResult, &iRow, &iCol, &pszErrMsg);
    if (rc != SQLITE_OK) {
        checkAndLogMsg ("SQLiteDatabase::execute", failureSqlStmt,
                        SQLiteFactory::getErrorAsString (rc),
                        pszStatement, (pszErrMsg != NULL ? pszErrMsg : ""));
        sqlite3_free (pszErrMsg);
        return -2;
    }

    if (iRow <= 0 || iCol <= 0) {
        return 0;
    }

    pTable->init (ppszResult, (unsigned int) iRow, (unsigned int) iCol);
    return 0;
}

unsigned int SQLiteDatabase::getNumberOfChanges (bool bRecursive)
{
    if (bRecursive) {
        return  sqlite3_total_changes (_pDB);
    }
    return sqlite3_changes (_pDB);
}

const char * SQLiteDatabase::getErrorMessage (void)
{
    return sqlite3_errmsg (_pDB);
}

Table * SQLiteDatabase::getTable()
{
    return new SQLiteTable();
}

PreparedStatement * SQLiteDatabase::prepare (const char *pszStatement)
{
    if (_pDB == NULL) {
        return NULL;
    }
    if (pszStatement == NULL) {
        checkAndLogMsg ("SQLiteDatabase::prepare", emptySqlStmt);
        return NULL;
    }

    SQLitePreparedStatement *pPrepStmt = new SQLitePreparedStatement();
    if (pPrepStmt == NULL) {
        checkAndLogMsg ("SQLiteDatabase::prepare", memoryExhausted);
        return NULL;
    }

    if (pPrepStmt->init (pszStatement, _pDB) < 0) {
        delete pPrepStmt;
        return NULL;
    }

    return pPrepStmt;
}

int SQLiteDatabase::beginTransaction()
{
    return execute ("BEGIN;");
}

int SQLiteDatabase::endTransaction()
{
    return execute ("COMMIT;");
}

int SQLiteDatabase::setAsynchronousMode (PragmaAsynchronousMode mode)
{
    switch (mode) {
        case AM_OFF:
            return setPragma (synchronous, "OFF");

        case AM_NORMAL:
            return setPragma (synchronous, "NORMAL");

        case AM_FULL:
            return setPragma (synchronous, "FULL");
    }
    return -1;
}

int SQLiteDatabase::setLockingMode (PragmaLockingMode mode)
{
    switch (mode) {
        case LM_NORMAL:
            return setPragma (locking_mode, "NORMAL");

        case LM_EXCLUSIVE:
            return setPragma (locking_mode, "EXCLUSIVE");
    }
    return -1;
}

int SQLiteDatabase::setPragma (Pragma pragma, const char *pszValue)
{
    if (_pDB == NULL || pszValue == NULL) {
        return -1;
    }

    char sql[255];
    memset (sql, '\0', 255);
    sprintf (sql, "PRAGMA %s = %s;", SQLITE_PRAGMAS[pragma], pszValue);

    return execute (sql);
}

