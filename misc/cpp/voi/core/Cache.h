/*
 * Cache.h
 *
 * This file is part of the IHMC Voi Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on Febraury 15, 2017
 */

#ifndef INCL_VOI_CACHE_H
#define INCL_VOI_CACHE_H

#include "FTypes.h"
#include "InformationObject.h"

#include "Database.h"
#include "Mutex.h"

namespace IHMC_VOI
{
    class MetadataInterface;

    struct EntityInfo
    {
        EntityInfo (void);
        ~EntityInfo (void);

        bool valid (void) const;

        int64 _i64SourceTimeStamp;
        NOMADSUtil::String _objectId;
        NOMADSUtil::String _instanceId;
    };

    class Cache
    {
        public:
            explicit Cache (const char *pszSessionId);
            ~Cache (void);

            int init (void);

            int add (const char *pszNodeId, EntityInfo &ei, MetadataInterface *pMetadata, const void *pData, int64 i64DataLen);
            int getPreviousVersions (const char *pszNodeId, const char *pszObjectId, unsigned int uiHistoryLen, InformationObjects &history) const;

        private:
            mutable NOMADSUtil::Mutex _m;
            const NOMADSUtil::String _sessionId;
            IHMC_MISC::PreparedStatement *_pInsertObjStmt;
            IHMC_MISC::PreparedStatement *_pInsertInstStmt;
            IHMC_MISC::PreparedStatement *_pInsertPeerStmt;
            IHMC_MISC::PreparedStatement *_pGetHistoryStmt;
            IHMC_MISC::SQLiteDatabase _db;
    };
}

#endif  /* INCL_VOI_CACHE_H */

