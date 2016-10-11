/* 
 * SchedulerCache.h
 *
 * This file is part of the IHMC DSPro Library/Component
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
 * Created on February 25, 2014, 5:48 PM
 */

#ifndef INCL_SCHEDULER_CACHE_H
#define	INCL_SCHEDULER_CACHE_H

#include "Mutex.h"
#include  "StrClass.h"

namespace IHMC_ACI
{
    class PropertyStoreInterface;

    class SchedulerCache
    {
        public:
            SchedulerCache (PropertyStoreInterface *pPropertyStore);
            virtual ~SchedulerCache (void);

            NOMADSUtil::String getLatestMessageIdPushedToTarget (const char *pszTarget);
            int setLatestMessageIdPushedToTarget (const char *pszTarget, const char *pszLatestMessageId);
            int resetLatestMessageIdPushedToTarget (const char *pszTarget);

            int64 getMostRecentMessageTimestamp (const char *pszTarget, const char *pszObjectId);
            int setMostRecentMessageTimestamp (const char *pszTarget, const char *pszObjectId, int64 i64Timestamp);

        private:
            NOMADSUtil::String getLatestMessageIdPushedToTargetInternal (const char *pszTarget);
            int64 getMostRecentMessageTimestampInternal (const char *pszTarget, const char *pszObjectId);

        private:
            static const char * LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY;
            static const char * LATEST_MESSAGE_TO_TARGET_SOURCE_TIMESTAMP_PROPERTY;

            PropertyStoreInterface *_pPropertyStore;
            NOMADSUtil::Mutex _m;
    };
}

#endif	/* SCHEDULERCACHE_H */

