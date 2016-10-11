/**
 * MatchmakingLogListener.h
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
 * Author: Giacomo Benincasa    gbenincasa@ihmc.us
 * Created on September 20, 2010, 11:21 PM
 */

#ifndef MATCHMAKING_LOG_LISTENER_H
#define	MATCHMAKING_LOG_LISTENER_H

namespace IHMC_ACI
{
    class MatchmakingLogListener
    {
        public:
            MatchmakingLogListener (void);
            virtual ~MatchmakingLogListener (void);

            virtual bool informationMatched (const char *pszLocalNodeID, const char *pszPeerNodeID,
                                             const char *pszMatchedObjectID, const char *pszMatchedObjectName,
                                             const char **ppszRankDescriptors, float *pRanks, float *pWeights,
                                             uint8 ui8Len, const char *pszComment, const char *pszOperation) = 0;

            virtual bool informationSkipped (const char *pszLocalNodeID, const char *pszPeerNodeID,
                                             const char *pszSkippedObjectID, const char *pszSkippedObjectName,
                                             const char **ppszRankDescriptors, float *pRranks, float *pWeights,
                                             uint8 ui8Len, const char *pszComment, const char *pszOperation) = 0;
    };

    inline MatchmakingLogListener::MatchmakingLogListener (void)
    {
    }

    inline MatchmakingLogListener::~MatchmakingLogListener (void)
    {
    }
}

#endif	// MATCHMAKING_LOG_LISTENER_H

