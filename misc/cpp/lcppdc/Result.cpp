/*
  * Result.cpp
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

#include "Result.h"

#include "LCppDCDefs.h"

#include "Logger.h"
#include "NLFLib.h"

#include "sqlite3.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

using namespace IHMC_MISC;
using namespace NOMADSUtil;

#define unitializedStmt Logger::L_SevereError, "_pStmt has not been initialized\n"

//=============================================================================
// SQLiteStatementRow
//=============================================================================

SQLiteStatementRow::SQLiteStatementRow()
{
    init (NULL, 0, 0, 0); 
}

SQLiteStatementRow::~SQLiteStatementRow()
{
}

void  SQLiteStatementRow::init (const char **ppszResults, unsigned int uiRows,
                                unsigned int uiColumns, unsigned int uiRowIndex)
{
    _ppszResults = ppszResults;
    _uiRows = uiRows;
    _uiColumns = uiColumns;
    _uiRowIndex = uiRowIndex;
}

unsigned int SQLiteStatementRow::getColumnCount()
{
    return _uiRows;
}

int SQLiteStatementRow::getValue (unsigned short usColumnIdx, bool &bVal)
{
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    int iVal;
    if (sscanf (pszVal, "%d", &iVal) != 1) {
        return -2;
    }

    if (iVal == TRUE_INT_VALUE) {
        bVal = true;
    }
    else if (iVal == FALSE_INT_VALUE) {
        bVal = false;
    }
    else {
        checkAndLogMsg ("SQLiteStatementRow::getValue", Logger::L_SevereError,
                        "bool value not in the proper format\n");
        return -3;
    }
    return 0;
}

int  SQLiteStatementRow::getValue (unsigned short usColumnIdx, int8 &i8Val)
{
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    int iVal;
    if (sscanf (pszVal, "%d", &iVal) != 1) {
        return -2;
    }

    i8Val = (int8) iVal;
    return 0;
}

int  SQLiteStatementRow::getValue (unsigned short usColumnIdx, int16 &i16Val)
{
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    int iVal;
    if (sscanf (pszVal, "%d", &iVal) != 1) {
        return -2;
    }

    i16Val = (int16) iVal;
    return 0;
}

int  SQLiteStatementRow::getValue (unsigned short usColumnIdx, int32 &i32Val)
{
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    int iVal;
    if (sscanf (pszVal, "%d", &iVal) != 1) {
        return -2;
    }

    i32Val = (int32) iVal;
    return 0;
}

int  SQLiteStatementRow::getValue (unsigned short usColumnIdx, int64 &i64Val)
{
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    i64Val = atoi64 (pszVal);
    return 0;
}

int  SQLiteStatementRow::getValue (unsigned short usColumnIdx, uint8 &ui8Val)
{
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    int iVal;
    if (sscanf (pszVal, "%d", &iVal) != 1) {
        return -2;
    }
    if (iVal < 0) {
        return -3;
    }

    ui8Val = (uint8) iVal;
    return 0;
}

int  SQLiteStatementRow::getValue (unsigned short usColumnIdx, uint16 &ui16Val)
{
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    int iVal;
    if (sscanf (pszVal, "%d", &iVal) != 1) {
        return -2;
    }
    if (iVal < 0) {
        return -3;
    }

    ui16Val = (uint16) iVal;
    return 0;
}

int  SQLiteStatementRow::getValue (unsigned short usColumnIdx, uint32 &ui32Val)
{
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    ui32Val = atoui32 (pszVal);
    return 0;
}

int  SQLiteStatementRow::getValue (unsigned short usColumnIdx, uint64 &ui64Val)
{
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    char *pszBo;
    double d = strtod (pszVal, &pszBo);
    ui64Val = (uint64) d;
    return 0;
}

int SQLiteStatementRow::getValue (unsigned short usColumnIdx, float &fVal)
{
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    char *pszBo;
    fVal = (float) strtod (pszVal, &pszBo);
    return 0;
}

int SQLiteStatementRow::getValue (unsigned short usColumnIdx, double &dVal)
{
     const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    char *pszBo;
    dVal = strtod (pszVal, &pszBo);
    return 0;
}

int  SQLiteStatementRow::getValue (unsigned short usColumnIdx, char **ppszVal)
{
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }

    *ppszVal = strDup ((char *) pszVal);
    return 0;
}

int SQLiteStatementRow::getValue (unsigned short usColumnIdx, void **ppBuf, int &iLen)
{
    iLen = 0;
    const char *pszVal = getValueAsString (usColumnIdx);
    if (pszVal == NULL) {
        return -1;
    }
    iLen = (int) strlen (pszVal);
    if (iLen > 0) {
        *ppBuf = calloc (iLen+1, 1);
    }
    memcpy (*ppBuf, pszVal, iLen);

    // TODO: implement this!  How does it work with a buffer which contains only \0s???
    return 0;
}

//=============================================================================
// SQLitePreparedStatementRow
//=============================================================================

SQLitePreparedStatementRow::SQLitePreparedStatementRow()
{
    _pStmt = NULL;
}

SQLitePreparedStatementRow::~SQLitePreparedStatementRow()
{
}

void  SQLitePreparedStatementRow::init (sqlite3_stmt *pStmt)
{
    _pStmt = pStmt;
}

unsigned int SQLitePreparedStatementRow::getColumnCount()
{
    if (_pStmt == NULL) {
        return 0;
    }
    return sqlite3_data_count (_pStmt);
}

int  SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, bool &bVal)
{
    if (_pStmt == NULL) {
        return -1;
    }
    int iVal = (uint32) sqlite3_column_int (_pStmt, usColumnIdx);
    if (iVal == TRUE_INT_VALUE) {
        bVal = true;
    }
    else if (iVal == FALSE_INT_VALUE) {
        bVal = false;
    }
    else {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue",Logger::L_SevereError,
                        "bool value not in the proper format\n");
    }
    return bVal;
}

int  SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, int8 &i8Val)
{
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (1)", unitializedStmt);
        return -1;
    }
    i8Val = (int8) sqlite3_column_int (_pStmt, usColumnIdx);
    return 0;
}

int  SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, int16 &i16Val)
{
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (2)", unitializedStmt);
        return -1;
    }
    i16Val = (int16) sqlite3_column_int (_pStmt, usColumnIdx);
    return 0;
}

int  SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, int32 &i32Val)
{
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (3)", unitializedStmt);
        return -1;
    }
    i32Val = (int32) sqlite3_column_int (_pStmt, usColumnIdx);
    return 0;
}

int  SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, int64 &i64Val)
{
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (4)", unitializedStmt);
        return -1;
    }
    i64Val = (int64) sqlite3_column_int64 (_pStmt, usColumnIdx);
    return 0;
}

int  SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, uint8 &ui8Val)
{
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (5)", unitializedStmt);
        return -1;
    }
    ui8Val = (uint8) sqlite3_column_int (_pStmt, usColumnIdx);
    return 0;
}

int  SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, uint16 &ui16Val)
{
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (6)", unitializedStmt);
        return -1;
    }
    ui16Val = (uint16) sqlite3_column_int (_pStmt, usColumnIdx);
    return 0;
}

int  SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, uint32 &ui32Val)
{
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (7)", unitializedStmt);
        return -1;
    }
    ui32Val = (uint32) sqlite3_column_int (_pStmt, usColumnIdx);
    return 0;
}

int  SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, uint64 &ui64Val)
{
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (8)", unitializedStmt);
        return -1;
    }
    ui64Val = (uint64) sqlite3_column_int64 (_pStmt, usColumnIdx);
    return 0;
}

int SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, float &f64Val)
{
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (9)", unitializedStmt);
        return -1;
    }
    f64Val = (float) sqlite3_column_double (_pStmt, usColumnIdx);
    return 0;
}

int SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, double &dVal)
{
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (10)", unitializedStmt);
        return -1;
    }
    dVal = sqlite3_column_double (_pStmt, usColumnIdx);
    return 0;
}

int  SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, char **ppszVal)
{
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (11)", unitializedStmt);
        return -1;
    }

    *ppszVal = NULL;

    switch (sqlite3_column_type (_pStmt, usColumnIdx)) {
        case SQLITE_INTEGER:
        {
            int64 i64Value = sqlite3_column_int64 (_pStmt, usColumnIdx);
            *ppszVal = (char *) calloc (22, sizeof (char));
            if (*ppszVal == NULL) {
                return -1;
            }
            *ppszVal = i64toa (*ppszVal, i64Value);
            return 0;
        }

        case SQLITE_FLOAT:
        {
            break;
        }

        case SQLITE_BLOB:
        {
            int iLen = sqlite3_column_bytes (_pStmt, usColumnIdx);
            if (iLen < 0) {
               return -1;
            }
            if (iLen == 0) {
                ppszVal = NULL;
                return -1;
            }
            *ppszVal = (char *) calloc (iLen + 1, 1);  // +1 to store the \0 (calloc sets all
                                                       // the bytes to \0 so we do not need to
                                                       // manually do it)
            if (*ppszVal == NULL) {
                checkAndLogMsg ("SQLitePreparedStatementRow::getValue (11)", memoryExhausted);
                 return -2;
            }

            const void *pBlob = sqlite3_column_blob (_pStmt, usColumnIdx);
            if (pBlob == NULL) {
                free (*ppszVal);
                return -3;
            }

            memcpy (*ppszVal, pBlob, iLen);
            return 0;
        }

        case SQLITE_NULL:
        {
            ppszVal = NULL;
            return 0;
        }

        case SQLITE_TEXT:
        {
            *ppszVal = strDup ((char *) sqlite3_column_text (_pStmt, usColumnIdx));
            return 0;
        }

        default:
            return -1;
    }

    return -1;
}

int SQLitePreparedStatementRow::getValue (unsigned short usColumnIdx, void **ppBuf,
                                          int &iLen)
{
    iLen = 0;
    if (_pStmt == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (12)", unitializedStmt);
        return -1;
    }

    iLen = sqlite3_column_bytes (_pStmt, usColumnIdx);
    if (iLen < 0) {
        iLen = 0;
        return -1;
    }
    if (iLen == 0) {
        ppBuf = NULL;
        iLen = 0;
        return -1;
    }

    *ppBuf = malloc (iLen);
    if (*ppBuf == NULL) {
        checkAndLogMsg ("SQLitePreparedStatementRow::getValue (12)", memoryExhausted);
        iLen = 0;
        return -2;
    }

    const void *pBlob = sqlite3_column_blob (_pStmt, usColumnIdx);
    if (pBlob == NULL) {
        free (*ppBuf);
        iLen = 0;
        return -3;
    }

    memcpy (*ppBuf, pBlob, iLen);
    return 0;
}

//=============================================================================
// AbstractSQLiteTable
//=============================================================================

AbstractSQLiteTable::AbstractSQLiteTable()
{
    init (NULL, 0, 0);
}

AbstractSQLiteTable::AbstractSQLiteTable (char **ppszResults, unsigned int uiRows, unsigned int uiColumns)
{
    init (ppszResults, uiRows, uiColumns);
}

AbstractSQLiteTable::~AbstractSQLiteTable()
{
}

void AbstractSQLiteTable::init (char **ppszResults, unsigned int uiRows, unsigned int uiColumns)
{
    _ppszResults = ppszResults;
    _uiRows = uiRows;
    _uiColumns = uiColumns;
    _uiRowIndex = (unsigned int) 1;
}

//=============================================================================
// SQLiteTable
//=============================================================================

SQLiteTable::SQLiteTable()
{
}

SQLiteTable::SQLiteTable (char **ppszResults, unsigned int uiRows, unsigned int uiColumns)
    : AbstractSQLiteTable (ppszResults, uiRows, uiColumns)
{
}

SQLiteTable::~SQLiteTable()
{
}

Row * SQLiteTable::getRow (void)
{
    return new SQLiteStatementRow();
}

bool SQLiteTable::next (Row *pRow)
{
    return nextInternal ((SQLiteStatementRow *)pRow);
}

bool SQLiteTable::nextInternal (SQLiteStatementRow *pRow)
{
    if (_uiRowIndex > _uiRows) {
        return false;
    }

    if (pRow != NULL) {
        pRow->init ((const char **)_ppszResults, _uiRows, _uiColumns, _uiRowIndex);
    }
    _uiRowIndex++;
    return true;
}


