/*
 * Result.h
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
 * Created on November 23, 2011, 12:00 PM
 */

#ifndef INCL_RESULT_H
#define INCL_RESULT_H

#include "FTypes.h"

#include <stddef.h>

struct sqlite3_stmt;

namespace IHMC_MISC
{
    class Row
    {
        public:
            virtual ~Row (void) {}

            /**
             * Returns the number of columns in the current row.
             * If the statement has not yet been executed, it returns 0.
             */
            virtual unsigned int getColumnCount (void) = 0;

            virtual int getValue (unsigned short usColumnIdx, bool &bVal) = 0;

            virtual int getValue (unsigned short usColumnIdx, int8 &i8Val) = 0;
            virtual int getValue (unsigned short usColumnIdx, int16 &i16Val) = 0;
            virtual int getValue (unsigned short usColumnIdx, int32 &i32Val) = 0;
            virtual int getValue (unsigned short usColumnIdx, int64 &i64Val) = 0;

            virtual int getValue (unsigned short usColumnIdx, uint8 &ui8Val) = 0;
            virtual int getValue (unsigned short usColumnIdx, uint16 &ui16Val) = 0;
            virtual int getValue (unsigned short usColumnIdx, uint32 &ui32Val) = 0;
            virtual int getValue (unsigned short usColumnIdx, uint64 &ui64Val) = 0;

            virtual int getValue (unsigned short usColumnIdx, float &f64Val) = 0;
            virtual int getValue (unsigned short usColumnIdx, double &dVal) = 0;

            virtual int getValue (unsigned short usColumnIdx, char **ppszVal) = 0;

            virtual int getValue (unsigned short usColumnIdx, void **ppBuf, int &iLen) = 0;

        protected:
            Row (void) {};
    };

    class SQLiteStatementRow : public Row
    {
        public:
            virtual ~SQLiteStatementRow (void);

            void init (const char **ppszResults,
                       unsigned int uiRows, unsigned int uiColumns,
                       unsigned int uiRowIndex);

            unsigned int getColumnCount (void);

            int getValue (unsigned short usColumnIdx, bool &bVal);

            int getValue (unsigned short usColumnIdx, int8 &i8Val);
            int getValue (unsigned short usColumnIdx, int16 &i16Val);
            int getValue (unsigned short usColumnIdx, int32 &i32Val);
            int getValue (unsigned short usColumnIdx, int64 &i64Val);

            int getValue (unsigned short usColumnIdx, uint8 &ui8Val);
            int getValue (unsigned short usColumnIdx, uint16 &ui16Val);
            int getValue (unsigned short usColumnIdx, uint32 &ui32Val);
            int getValue (unsigned short usColumnIdx, uint64 &ui64Val);

            int getValue (unsigned short usColumnIdx, float &f64Val);
            int getValue (unsigned short usColumnIdx, double &dVal);

            /**
             * NOTE: the caller has to deallocate ppszVal and ppBuf
             */
            int getValue (unsigned short usColumnIdx, char **ppszVal);
            int getValue (unsigned short usColumnIdx, void **ppBuf, int &iLen);

        private:
            friend class SQLiteTable;

            SQLiteStatementRow (void);

            const char * getValueAsString (unsigned short usColumnIdx);

            unsigned int _uiColumns;
            unsigned int _uiRows;
            unsigned int _uiRowIndex;
            const char **_ppszResults;
    };

    class SQLitePreparedStatementRow : public Row
    {
        public:
            virtual ~SQLitePreparedStatementRow (void);

            void init (sqlite3_stmt *pStmt);

            unsigned int getColumnCount (void);

            int getValue (unsigned short usColumnIdx, bool &bVal);

            int getValue (unsigned short usColumnIdx, int8 &i8Val);
            int getValue (unsigned short usColumnIdx, int16 &i16Val);
            int getValue (unsigned short usColumnIdx, int32 &i32Val);
            int getValue (unsigned short usColumnIdx, int64 &i64Val);

            int getValue (unsigned short usColumnIdx, uint8 &ui8Val);
            int getValue (unsigned short usColumnIdx, uint16 &ui16Val);
            int getValue (unsigned short usColumnIdx, uint32 &ui32Val);
            int getValue (unsigned short usColumnIdx, uint64 &ui64Val);

            int getValue (unsigned short usColumnIdx, float &f64Val);
            int getValue (unsigned short usColumnIdx, double &dVal);

            int getValue (unsigned short usColumnIdx, char **ppszVal);

            int getValue (unsigned short usColumnIdx, void **ppBuf, int &iLen);

        private:
            friend class SQLitePreparedStatement;

            SQLitePreparedStatementRow (void);

            sqlite3_stmt *_pStmt;
    };

    class Table
    {
        public:
            virtual ~Table (void);
            virtual Row * getRow (void) = 0;

            /**
             * Moves to the next output row and returns true if it exists.
             * Returns false otherwise.
             * If pRow is not null it will be filled with the proper values
             * for this row
             */
            virtual bool next (Row *pRow) = 0;
    };

    class AbstractSQLiteTable : public Table
    {
        public:
            virtual ~AbstractSQLiteTable (void);

            void init (char **ppszResults, unsigned int uiRows, unsigned int uiColumns);

        protected:
            AbstractSQLiteTable (void);
            AbstractSQLiteTable (char **ppszResults, unsigned int uiRows, unsigned int uiColumns);

            unsigned int _uiRowIndex;
            unsigned int _uiRows;
            unsigned int _uiColumns;
            char **_ppszResults;
    };

    class SQLiteTable : public AbstractSQLiteTable
    {
        public:
            virtual ~SQLiteTable (void);

            Row * getRow (void);
            bool next (Row *pRow);

        private:
            /* SQLiteDatabase can be instantiated only by SQLiteDatabase */
            friend class SQLiteDatabase;

            SQLiteTable (void);
            SQLiteTable (char **ppszResults, unsigned int uiRows, unsigned int uiColumns);

            bool nextInternal (SQLiteStatementRow *pRow);
    };

    inline const char * SQLiteStatementRow::getValueAsString (unsigned short usColumnIdx)
    {
        if (_ppszResults == NULL || usColumnIdx >= _uiColumns || _uiRowIndex > _uiRows) {
            return NULL;
        }
        return _ppszResults[(_uiRowIndex*_uiColumns)+usColumnIdx];
    }

    inline Table::~Table (void)
    {
    }
}

#endif // INCL_RESULT_H


