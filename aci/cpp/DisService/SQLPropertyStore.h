/*
 * SQLPropertyStore.h
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

#ifndef INCL_SQL_PROPERTY_STORE_H
#define INCL_SQL_PROPERTY_STORE_H

#include "Mutex.h"
#include "StrClass.h"

#include "PropertyStoreInterface.h"

#include <stddef.h>

namespace IHMC_MISC
{
    class PreparedStatement;
}

namespace IHMC_ACI
{
    class SQLPropertyStore : public PropertyStoreInterface
    {
        public:
            SQLPropertyStore (void);
            virtual ~SQLPropertyStore (void);

            virtual int init (const char *pszStorageFile = NULL);

            virtual int set (const char *pszNodeID, const char *pszAttr, const char *pszValue);
            virtual NOMADSUtil::String get (const char *pszNodeID, const char *pszAttr);
            virtual int remove (const char *pszNodeID, const char *pszAttr);
            virtual int update (const char *pszNodeID, const char *pszAttr, const char *pszNewValue);

            virtual void clear (void);

        public:
            static const NOMADSUtil::String TABLE_NAME;
            static const NOMADSUtil::String NODE_ID_COL;
            static const NOMADSUtil::String ATTR_COL;
            static const NOMADSUtil::String VALUE_COL;

        private:
            int setInternal (IHMC_MISC::PreparedStatement *pPreparedStmt, const char *pszNodeID,
                             const char *pszAttr, const char *pszValue);

        private:
            NOMADSUtil::Mutex _m;
            NOMADSUtil::String _storageFile;
            IHMC_MISC::PreparedStatement *_ppsSetProperty;
            IHMC_MISC::PreparedStatement *_ppsGetProperty;
            IHMC_MISC::PreparedStatement *_ppsRemoveProperty;
            IHMC_MISC::PreparedStatement *_ppsUpdateProperty;
            IHMC_MISC::PreparedStatement *_ppsDeleteAll;
    };
}

#endif   // #ifndef INCL_SQL_PROPERTY_STORE_H
