/*
 * SQLiteFactory.h
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
 * Created on June 16, 2010, 5:30 PM
 */

#ifndef INCL_SQLITE_FACTORY_H
#define	INCL_SQLITE_FACTORY_H

struct sqlite3;

namespace IHMC_MISC
{
    class SQLiteFactory
    {
        public:
            virtual ~SQLiteFactory (void);

            static void close (void);

            /**
             * Returns an instance of sqlite3.  If the database has not been
             * opened or created it is opened or created.
             * If pszFileName is not specified the database is open in memory.
             */
            static sqlite3 * getInstance (bool bTrace = false);
            static sqlite3 * getInstance (const char *pszFileName, bool bTrace = false);
            static const char * getErrorAsString (int errorCode);

        protected:
            SQLiteFactory (void);
            static sqlite3 *_pDB;
    };
}

#endif	// INCL_SQLITE_FACTORY_H
