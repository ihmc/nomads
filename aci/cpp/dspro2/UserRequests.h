/* 
 * UserRequests.h
 *
 * Cache to memorize the data that was explicitely requested by the user
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on January 14, 2012, 2:07 AM
 */

#ifndef INCL_REQUESTED_DATA_CACHE_H
#define	INCL_REQUESTED_DATA_CACHE_H

#include "LoggingMutex.h"
#include "RangeDLList.h"
#include "StrClass.h"
#include "StringHashset.h"
#include "StringHashtable.h"
#include "UInt32Hashtable.h"

namespace IHMC_ACI
{
    class UserRequests
    {
        public:
            UserRequests (void);
            virtual ~UserRequests (void);

            bool contains (const char *pszMsgId, NOMADSUtil::String &queryId);
            bool contains (const char *pszGroupName, const char *pszSenderNoded,
                           uint32 ui32MsgSeqId, NOMADSUtil::String &queryId);

            /**
             * Returns
             * - 0 if the message ID was inserted,
             * - 1 if the message ID had already been stored, therefore the ID
             *   was not added because it was not necessary
             * - A negative number if the message ID was not stored because of
             *   an error
             */
            int put (const char *pszMsgId);
            int put (const char *pszGroupName, const char *pszSenderNoded, uint32 ui32MsgSeqId);

            int put (const char *pszMsgId, const char *pszQueryId);
            int put (const char *pszGroupName, const char *pszSenderNoded, uint32 ui32MsgSeqId, const char *pszQueryId);

            int remove (const char *pszMsgId);
            int remove (const char *pszGroupName, const char *pszSenderNoded, uint32 ui32MsgSeqId);

        private:
            struct BySeqId
            {
                BySeqId (void);
                ~BySeqId (void);

                NOMADSUtil::UInt32Hashtable<NOMADSUtil::String> seqIdToQueryId;
                NOMADSUtil::UInt32RangeDLList sedIds;
            };

            typedef NOMADSUtil::StringHashtable<BySeqId> BySender;
            NOMADSUtil::StringHashtable<BySender> _byGroupName;

            NOMADSUtil::LoggingMutex _m;
            NOMADSUtil::StringHashset _hs;
    };
}

#endif	/* INCL_REQUESTED_DATA_CACHE_H */

