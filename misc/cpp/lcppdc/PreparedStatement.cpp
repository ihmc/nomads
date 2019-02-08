/*
 * PreparedStatement.cpp
 *
 * This file is part of the IHMC Database Connectivity Library.
 * Copyright (c) 1993-2016 IHMC.
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
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#include "PreparedStatement.h"

#include "LCppDCDefs.h"
#include "SQLiteFactory.h"
#include "Result.h"

#include "Logger.h"

#include "sqlite3.h"

#include <string.h>

using namespace IHMC_MISC;
using namespace NOMADSUtil;

SQLitePreparedStatement::SQLitePreparedStatement()
{
    _pPrepStmt = NULL;
    _pDB = NULL;
}

SQLitePreparedStatement::~SQLitePreparedStatement()
{
    if (_pPrepStmt != NULL) {
        sqlite3_finalize (_pPrepStmt);
        _pPrepStmt = NULL;
    }
    _pDB = NULL;
}

int SQLitePreparedStatement::init (const char *pszStmt, sqlite3 *pDB)
{
    if (pszStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::prepare", emptySqlStmt);
        return -1;
    }

    int rc = sqlite3_prepare_v2 (pDB, pszStmt, (int) strlen (pszStmt), &_pPrepStmt, NULL);
    if ((rc != SQLITE_OK) || (_pPrepStmt == NULL)) {
        checkAndLogMsg ("SQLitePreparedStatement::prepare", failureSqlStmt, pszStmt,
                        SQLiteFactory::getErrorAsString (rc), "");
        if (_pPrepStmt != NULL) {
            sqlite3_clear_bindings (_pPrepStmt);
            _pPrepStmt = NULL;
        }
        return -2;
    }

    _pDB = pDB;
    checkAndLogMsg ("SQLitePreparedStatement::prepare", successfulSqlStmt, pszStmt);
    return 0;
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, const char *pszVal, bool bStaticValue)
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::bind", emptySqlStmt);
        return -1;
    }

    int rc = -1;
    int iLen = (pszVal == NULL ? 0 : (int) strlen (pszVal));
    if (iLen == 0) {
        rc = sqlite3_bind_null (_pPrepStmt, (int) usColumnIdx);
    }
    else {
        rc = sqlite3_bind_text (_pPrepStmt, (int) usColumnIdx, pszVal,
                                iLen, bStaticValue ? SQLITE_STATIC : SQLITE_TRANSIENT);
    }

    if (rc != SQLITE_OK) {
        checkAndLogMsg ("SQLitePreparedStatement::bind", bindingError,
                        usColumnIdx, SQLiteFactory::getErrorAsString (rc));
        return -usColumnIdx;
    }

    return 0;
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, uint8 ui8Val)
{
    return bind (usColumnIdx, static_cast<uint32>(ui8Val));
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, uint16 ui16Val)
{
    return bind (usColumnIdx, static_cast<uint32>(ui16Val));
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, uint32 ui32Val)
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::bind", emptySqlStmt);
        return -1;
    }

    int rc = sqlite3_bind_int (_pPrepStmt, (int) usColumnIdx, ui32Val);
    if (rc != SQLITE_OK) {
        checkAndLogMsg ("SQLitePreparedStatement::bind", bindingError,
                        usColumnIdx, SQLiteFactory::getErrorAsString (rc));
        return -usColumnIdx;
    }

    return 0;
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, uint64 ui64Val)
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (uint64)", emptySqlStmt);
        return -1;
    }

    int rc = sqlite3_bind_int64 (_pPrepStmt, (int) usColumnIdx, ui64Val);
    if (rc != SQLITE_OK) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (uint64)", bindingError,
                        usColumnIdx, SQLiteFactory::getErrorAsString (rc));
        return -usColumnIdx;
    }

    return 0;
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, int8 i8Val)
{
    return bind (usColumnIdx, static_cast<int32>(i8Val));
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, int16 i16Val)
{
    return bind (usColumnIdx, static_cast<int32>(i16Val));
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, int32 i32Val)
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (int32)", emptySqlStmt);
        return -1;
    }

    int rc = sqlite3_bind_int (_pPrepStmt, (int) usColumnIdx, i32Val);
    if (rc != SQLITE_OK) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (int32)", bindingError,
                        usColumnIdx, SQLiteFactory::getErrorAsString (rc));
        return -usColumnIdx;
    }

    return 0;
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, int64 i64Val)
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (int64)", emptySqlStmt);
        return -1;
    }

    int rc = sqlite3_bind_int64 (_pPrepStmt, (int) usColumnIdx, i64Val);
    if (rc != SQLITE_OK) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (int64)", bindingError,
                        usColumnIdx, SQLiteFactory::getErrorAsString (rc));
        return -usColumnIdx;
    }

    return 0;
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, float fVal)
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (float)", emptySqlStmt);
        return -1;
    }

    int rc = sqlite3_bind_double (_pPrepStmt, (int) usColumnIdx, fVal);
    if (rc != SQLITE_OK) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (float)", bindingError,
                        usColumnIdx, SQLiteFactory::getErrorAsString (rc));
        return -usColumnIdx;
    }

    return 0;
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, double dVal)
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (double)", emptySqlStmt);
        return -1;
    }

    int rc = sqlite3_bind_double (_pPrepStmt, (int) usColumnIdx, dVal);
    if (rc != SQLITE_OK) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (double)", bindingError,
                        usColumnIdx, SQLiteFactory::getErrorAsString (rc));
        return -usColumnIdx;
    }

    return 0;
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, bool bVal)
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (bool)", emptySqlStmt);
        return -1;
    }

    int rc = sqlite3_bind_int (_pPrepStmt, (int) usColumnIdx,
                               (bVal ? TRUE_INT_VALUE : FALSE_INT_VALUE));
    if (rc != SQLITE_OK) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (bool)", bindingError,
                        usColumnIdx, SQLiteFactory::getErrorAsString (rc));
        return -usColumnIdx;
    }

    return 0;
}

int SQLitePreparedStatement::bind (unsigned short usColumnIdx, const void *pBuf, int iLen, bool bStaticValue)
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (blob)", emptySqlStmt);
        return -1;
    }

    if (pBuf == NULL) {
        if (iLen > 0) {
            checkAndLogMsg ("SQLitePreparedStatement::bind (blob)", Logger::L_SevereError,
                            "buffer is null but buffer length is %d\n", iLen);
        }
    }
    else if (iLen == 0) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (blob)", Logger::L_SevereError,
                        "buffer is not null but buffer length is 0\n");
    }

    int rc = SQLITE_ERROR;
    if (iLen == 0) {
        rc = sqlite3_bind_null (_pPrepStmt, (int) usColumnIdx);
    }
    else {
        rc = sqlite3_bind_blob (_pPrepStmt, (int) usColumnIdx, pBuf,
                                iLen, bStaticValue ? SQLITE_STATIC : SQLITE_TRANSIENT);
    }

    if (rc != SQLITE_OK) {
        checkAndLogMsg ("SQLitePreparedStatement::bind (blob)", bindingError,
                        usColumnIdx, SQLiteFactory::getErrorAsString (rc));
        return -usColumnIdx;
    }

    return 0;
}

int SQLitePreparedStatement::bindNull (unsigned short usColumnIdx)
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::bindNull", emptySqlStmt);
        return -1;
    }

    int rc = sqlite3_bind_null (_pPrepStmt, (int) usColumnIdx);

    if (rc != SQLITE_OK) {
        checkAndLogMsg ("SQLitePreparedStatement::bindNull", bindingError,
                        usColumnIdx, SQLiteFactory::getErrorAsString (rc));
        return -usColumnIdx;
    }

    return 0;
}

Row * SQLitePreparedStatement::getRow()
{
    SQLitePreparedStatementRow *pRow = new SQLitePreparedStatementRow();
    if (pRow != NULL) {
        pRow->init (_pPrepStmt);
    }
    return pRow;
}

bool SQLitePreparedStatement::next (Row *pRow)
{
    return nextInternal ((SQLitePreparedStatementRow *)pRow);
}

bool SQLitePreparedStatement::nextInternal (SQLitePreparedStatementRow *pRow)
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::next", emptySqlStmt);
        return false;
    }

    int rc = sqlite3_step (_pPrepStmt);

    if (rc != SQLITE_ROW) {
        if (rc != SQLITE_DONE) {
            checkAndLogMsg ("SQLitePreparedStatement::next", failureSqlStmt,
                            SQLiteFactory::getErrorAsString (rc),
                            "PREPARED STATEMENT", "");
        }
        return false;
    }

    // Initialize row
    if (pRow != NULL) {
        pRow->init (_pPrepStmt);
    }

    return true;
}

int SQLitePreparedStatement::update()
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::update", emptySqlStmt);
        return -1;
    }

    int rc = sqlite3_step (_pPrepStmt);
    switch (rc) {
        case SQLITE_OK:
        case SQLITE_DONE:
            checkAndLogMsg ("SQLitePreparedStatement::update", Logger::L_LowDetailDebug,
                            "update run successfully\n");
            return 0;

        default: {
            const char *pszErrMsg = sqlite3_errmsg (_pDB);
            checkAndLogMsg ("SQLitePreparedStatement::update", failureSqlStmt,
                            SQLiteFactory::getErrorAsString (rc),
                            "PREPARED STATEMENT", (pszErrMsg == NULL ? "" : pszErrMsg));
            return -1;
        }
    }
}

void SQLitePreparedStatement::reset()
{
    if (_pPrepStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatement::reset", emptySqlStmt);
        return;
    }

    sqlite3_reset (_pPrepStmt);
}

