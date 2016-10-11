/*
 * SQLiteFactory.cpp
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

#include "SQLiteFactory.h"

#include "Logger.h"

#include "sqlite3.h"

#include <stdio.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_MISC;
using namespace NOMADSUtil;

sqlite3 * SQLiteFactory::_pDB = NULL;

void trace (void *, const char *pszSql)
{
    printf ("%s\n\n", pszSql);
}

SQLiteFactory::SQLiteFactory()
{
}

SQLiteFactory::~SQLiteFactory()
{
    if (_pDB != NULL) {
        sqlite3_close (_pDB);
    }
}

void SQLiteFactory::close()
{
    if (_pDB != NULL) {
        sqlite3_close (_pDB);
        _pDB = NULL;
    }
}

sqlite3 * SQLiteFactory::getInstance (bool bTrace)
{
    return getInstance (":memory:", bTrace);
}

sqlite3 *  SQLiteFactory::getInstance (const char *pszFileName, bool bTrace)
{
    if (_pDB == NULL) {
        if (pszFileName == NULL) {
            checkAndLogMsg ("SQLiteFactory::getInstance", Logger::L_SevereError,
                            "Can't open the database of name \"NULL\".\n");
            return NULL;
        }
        int rc = sqlite3_open (pszFileName, &_pDB);
        if (rc != SQLITE_OK) {
            checkAndLogMsg ("SQLiteFactory::getInstance", Logger::L_SevereError,
                            "Can't open the database %s\n", SQLiteFactory::getErrorAsString (rc));
            sqlite3_close (_pDB);
            _pDB = NULL;
        }
        if (bTrace) {
            sqlite3_trace (_pDB, trace, NULL);
        }
    }
    return _pDB;
}

const char * SQLiteFactory::getErrorAsString (int errorCode)
{
    switch (errorCode) {
        case SQLITE_OK: return "SQLITE_OK";
        case SQLITE_ERROR: return "SQLITE_ERROR";
        case SQLITE_INTERNAL: return "SQLITE_INTERNAL";
        case SQLITE_PERM: return "SQLITE_PERM";
        case SQLITE_ABORT: return "SQLITE_ABORT";
        case SQLITE_BUSY: return "SQLITE_BUSY";
        case SQLITE_LOCKED: return "SQLITE_LOCKED";
        case SQLITE_NOMEM: return "SQLITE_NOMEM";
        case SQLITE_READONLY: return "SQLITE_READONLY";
        case SQLITE_INTERRUPT: return "SQLITE_INTERRUPT";
        case SQLITE_IOERR: return "SQLITE_IOERR";
        case SQLITE_CORRUPT: return "SQLITE_CORRUPT";
        case SQLITE_NOTFOUND: return "SQLITE_NOTFOUND";
        case SQLITE_FULL: return "SQLITE_FULL";
        case SQLITE_CANTOPEN: return "SQLITE_CANTOPEN";
        case SQLITE_PROTOCOL: return "SQLITE_PROTOCOL";
        case SQLITE_EMPTY: return "SQLITE_EMPTY";
        case SQLITE_SCHEMA: return "SQLITE_SCHEMA";
        case SQLITE_TOOBIG: return "SQLITE_TOOBIG";
        case SQLITE_CONSTRAINT: return "SQLITE_CONSTRAINT";
        case SQLITE_MISMATCH: return "SQLITE_MISMATCH";
        case SQLITE_MISUSE: return "SQLITE_MISUSE";
        case SQLITE_NOLFS: return "SQLITE_NOLFS";
        case SQLITE_AUTH: return "SQLITE_AUTH";
        case SQLITE_FORMAT: return "SQLITE_FORMAT";
        case SQLITE_RANGE: return "SQLITE_RANGE";
        case SQLITE_NOTADB: return "SQLITE_NOTADB";
        case SQLITE_ROW : return "SQLITE_ROW";
        case SQLITE_DONE:  return "SQLITE_DONE";
        default: return "UNKNOWN";
    }
}

