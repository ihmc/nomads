/*
  * PreparedStatement.h
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
  *
  * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
  * Created on November 23, 2011, 12:00 PM
  */

#ifndef INCL_PREPARED_STATEMENT_H
#define INCL_PREPARED_STATEMENT_H

#include "FTypes.h"
#include "Result.h"

struct sqlite3;
struct sqlite3_stmt;

namespace IHMC_MISC
{
    class Row;
    class SQLitePreparedStatementRow;

    class PreparedStatement : public Table
    {
        public:
            virtual ~PreparedStatement (void);

            virtual int bind (unsigned short usColumnIdx, const char *pszVal) = 0;
            virtual int bind (unsigned short usColumnIdx, uint8 ui8Val) = 0;
            virtual int bind (unsigned short usColumnIdx, uint16 ui16Val) = 0;
            virtual int bind (unsigned short usColumnIdx, uint32 ui32Val) = 0;
            virtual int bind (unsigned short usColumnIdx, uint64 ui64Val) = 0;
            virtual int bind (unsigned short usColumnIdx, int8 i8Val) = 0;
            virtual int bind (unsigned short usColumnIdx, int16 i16Val) = 0;
            virtual int bind (unsigned short usColumnIdx, int32 i32Val) = 0;
            virtual int bind (unsigned short usColumnIdx, int64 i64Val) = 0;
            virtual int bind (unsigned short usColumnIdx, float fVal) = 0;
            virtual int bind (unsigned short usColumnIdx, double dVal) = 0;
            virtual int bind (unsigned short usColumnIdx, bool bVal) = 0;
            virtual int bind (unsigned short usColumnIdx, void *pBuf, int iLen) = 0;

            virtual int bindNull (unsigned short usColumnIdx) = 0;

            /**
             * Returns 0 if the prepared statement was executed and
             * it did not return any output, a negative number otherwise.
             */
            virtual int update (void) = 0;

            /**
             * Resets the prepared statement
             */
            virtual void reset (void) = 0;

        protected:
            PreparedStatement (void);
    };

    class SQLitePreparedStatement : public PreparedStatement
    {
        public:
            SQLitePreparedStatement (void);
            virtual ~SQLitePreparedStatement (void);

            int init (const char *pszStmt, sqlite3 *pDB);

            int bind (unsigned short usColumnIdx, const char *pszVal);
            int bind (unsigned short usColumnIdx, uint8 ui8Val);
            int bind (unsigned short usColumnIdx, uint16 ui16Val);
            int bind (unsigned short usColumnIdx, uint32 ui32Val);
            int bind (unsigned short usColumnIdx, uint64 ui64Val);
            int bind (unsigned short usColumnIdx, int8 i8Val);
            int bind (unsigned short usColumnIdx, int16 i16Val);
            int bind (unsigned short usColumnIdx, int32 i32Val);
            int bind (unsigned short usColumnIdx, int64 i64Val);
            int bind (unsigned short usColumnIdx, float fVal);
            int bind (unsigned short usColumnIdx, double dVal);
            int bind (unsigned short usColumnIdx, bool bVal);
            int bind (unsigned short usColumnIdx, void *pBuf, int i16Len);

            int bindNull (unsigned short usColumnIdx);

            Row * getRow (void);
            bool next (Row *pRow);
            int update (void);

            void reset (void);

        private:
            bool nextInternal (SQLitePreparedStatementRow *pRow);

            sqlite3_stmt *_pPrepStmt;
            sqlite3 *_pDB;
    };

    inline PreparedStatement::PreparedStatement()
    {
    }

    inline PreparedStatement::~PreparedStatement()
    {
    }
}

#endif // INCL_PREPARED_STATEMENT_H


