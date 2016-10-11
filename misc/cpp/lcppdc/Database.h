/*
 * Database.h
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
 * Created on November 23, 2011, 3:19 AM
 */

#ifndef INCL_SQL_DATABASE_H
#define	INCL_SQL_DATABASE_H

#include "StrClass.h"

#include <stddef.h>

struct sqlite3;
struct sqlite3_stmt;

namespace IHMC_MISC
{
    class PreparedStatement;
    class AbstractSQLiteTable;
    class Table;

    class Database
    {
        public:
            enum Type
            {
                SQLite
            };

            virtual ~Database (void);

            static Database * getDatabase (Type type);
            static const char * getTypeAsString (Type type);

            Type getType (void) const;
            const char * getDBName (void) const;
            virtual const char * getErrorMessage (void) = 0;

            /**
             * if pszDBName is set to NULL the database will be opened in
             * memory if possible, otherwise an error is returned
             */
            virtual int open (const char *pszDBName=NULL) = 0;
            virtual int close (void) = 0;

            virtual int execute (const char *pszStatement) = 0;

            virtual int execute (const char *pszStatement, Table *pTable) = 0;

            /**
             * Return the number of rows that were changed or inserted or deleted by
             * the most recently completed SQL statement.
             * If bRecursive is set on false, changes caused by triggers or foreign
             * key actions will not be counted.
             */
            virtual unsigned int getNumberOfChanges (bool bRecursive=false) = 0;

            virtual Table * getTable (void) = 0;

            virtual PreparedStatement * prepare (const char *pszStatement) = 0;

            virtual int beginTransaction (void) = 0;
            virtual int endTransaction (void) = 0;

        protected:
            explicit Database (Type type);

        protected:
            NOMADSUtil::String _dbName;

        private:
            static Database *_pDB;
            const Type _type;
    };

    class SQLiteDatabase : public Database
    {
        public:
            enum Pragma {
                cache_size          = 0x00,    // Possible values: the cache size in KB
                journal_mode        = 0x01,    // Possible values: DELETE | TRUNCATE | PERSIST | MEMORY | WAL | OFF 
                journal_size_limit  = 0x03,    // Possible values: N (-1 means no limit)
                locking_mode        = 0x04,    // Possible values: NORMAL | EXCLUSIVE
                synchronous         = 0x05     // Possible values: OFF | NORMAL | FULL
            };

            SQLiteDatabase (void);
            virtual ~SQLiteDatabase (void);

            int open (const char *pszDBName);
            int close (void);

            int execute (const char *pszStatement);

            /**
             * NOTE: this may be expensive if the query returns a large
             *       table, PreparedStatements should be used when possible.
             *       The only advantage of obtaining the table is that it is
             *       possible to return the number of rows/columns.
             */
            int execute (const char *pszStatement, Table *pTable);

            unsigned int getNumberOfChanges (bool bRecursive=false);

            const char * getErrorMessage (void);
            Table * getTable (void);

            PreparedStatement * prepare (const char *pszStatement);

            int beginTransaction (void);
            int endTransaction (void);

            /**
             * Extracted from SQLite Documentation at
             * http://www.sqlite.org/pragma.html#pragma_synchronous
             * The DOCUMENTATION here reported is NOT COMPLETE, please refer to
             * the official one on the web!
             * 
             * When synchronous is FULL, the SQLite database engine will use the
             * xSync method of the VFS to ensure that all content is safely
             * written to the disk surface prior to continuing. If the operating
             * system crash will not corrupt the database.
             * FULL is _slower_.
             *
             * When synchronous is NORMAL, the SQLite database engine will still
             * sync at the most critical moments, but less often than in FULL mode.
             * There is a very small (though non-zero) chance that a power failure
             * could corrupt the database in NORMAL mode.
             *
             * With synchronous OFF, SQLite continues without syncing as soon as
             * it has handed data off to the operating system.
             * If the _application_ running SQLite crashes, the data will be safe,
             * but the database might become corrupted if the operating system
             * crashes or the computer loses power before that data has been
             * written to the disk surface. On the other hand, some operations
             * are as much as 50 or more times faster with synchronous OFF.
             */
            enum PragmaAsynchronousMode {
                AM_FULL    = 0x02,
                AM_NORMAL  = 0x01,
                AM_OFF     = 0x00
            };
            int setAsynchronousMode (PragmaAsynchronousMode mode);

            /**
             * Extracted from SQLite Documentation at
             * http://www.sqlite.org/pragma.html#pragma_locking_mode
             * The DOCUMENTATION here reported is NOT COMPLETE, please refer to
             * the official one on the web!
             * 
             * In NORMAL locking-mode a database connection unlocks the database
             * file at the conclusion of each read or write transaction.
             *
             * When the locking-mode is set to EXCLUSIVE, the database connection
             * never releases file-locks.
             * The first time the database is read in EXCLUSIVE mode, a shared
             * lock is obtained and held.
             * The first time the database is written, an exclusive lock is
             * obtained and held.
             *
             * Database locks obtained by a connection in EXCLUSIVE mode may be
             * released either by closing the database connection, or by setting
             * the locking-mode back to NORMAL _and_ then accessing the database
             * file (for read or write).
             * - Application wants to prevent other processes from accessing the
             *   database file.
             * - The number of system calls for filesystem operations is reduced,
             *    possibly resulting in a small performance increase.
             */
            enum PragmaLockingMode {
                LM_EXCLUSIVE    = 0x01,
                LM_NORMAL       = 0x00
            };
            int setLockingMode (PragmaLockingMode mode);

            int setPragma (Pragma pragma, const char *pszValue);

        private:
            static const char * SQLITE_PRAGMAS[];

            static const char * IN_MEMORY;

            int executeInternal (const char *pszStatement, AbstractSQLiteTable *pTable);

            sqlite3 *_pDB;
    };
}

#endif    // INCL_SQL_DATABASE_H

