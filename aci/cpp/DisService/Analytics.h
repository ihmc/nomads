/*
 * Analytics.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 3, 2011, 1:29 AM
 */

#ifndef INCL_ANALYTICS_H
#define	INCL_ANALYTICS_H

#include "SQLMessageHeaderStorage.h"

namespace IHMC_MISC
{
    class Database;
    class PreparedStatement;
}

namespace IHMC_ACI
{
    class Analytics : public SQLMessageHeaderStorage
    {
        public:
            Analytics (void);
            virtual ~Analytics (void);

            int init (void);

            /**
             * Returns the percentage of bytes that have been received
             * (rcvdBytes/totalBytes).
             *
             * i64TFirst: the arrival time of the first message/fragment
             *            matching the query
             * i64TLatest: the arrival time of the latest message/fragment
             *             matching the query
             * i64TotBytes: the total amount of _known_ bytes
             *
             * TODO: these methods are not yet fully implemented
             */
            float getPercentRcvdBytes (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId,
                                       int64 &i64TFirst, int64 &i64TLatest, int64 &i64TotBytes);

            float getPercentRcvdBytes (const char *pszGroupName, const char *pszSenderNodeId,
                                       int64 &i64TFirst, int64 &i64TLatest, int64 &i64TotBytes);

            float getPercentRcvdBytes (const char *pszGroupName,
                                       int64 &i64TFirst, int64 &i64TLatest, int64 &i64TotBytes);

            /**
             * ui32RcvdMsg: the number of messages, in pszGroupName, for which
             *              at least a fragment was received (average among the
             *              different senders)
             *
             * ui32MissingMsg: the number of messages for which no one fragment
             *                 was received (average among the senders).
             *
             * i64Time: the difference between the latest received message and
             *          the first one
             *
             * TODO: these method is not yet fully implemented
             */
            int getPercentMissingMessages (const char *pszGroupName, uint32 &ui32RcvdMsg,
                                           uint32 &ui32MissingMsg, int64 &i64Time);

        private:
            IHMC_MISC::PreparedStatement *_pGetSubscriptionStats;
            IHMC_MISC::PreparedStatement *_pPercRcvdBytes;
            IHMC_MISC::PreparedStatement *_pPercRcvdBytesByGrpSdr;
            IHMC_MISC::PreparedStatement *_pPercRcvdBytesByGrp;
            IHMC_MISC::PreparedStatement *_pPercMissMsgs;
    };
}

#endif	// INCL_ANALYTICS_H

