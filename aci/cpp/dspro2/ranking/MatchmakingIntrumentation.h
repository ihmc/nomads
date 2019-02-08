/*
 * MatchmakingIntrumentation.h
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
 * Created on June 26, 2012, 6:54 PM
 */

#ifndef INCL_MATCHMAKING_INTRUMENTATION_H
#define INCL_MATCHMAKING_INTRUMENTATION_H

#include "StrClass.h"
#include "PtrLList.h"

namespace IHMC_VOI
{
    struct Rank;
}

namespace IHMC_ACI
{
    class MatchmakingIntrumentation
    {
        public:
            MatchmakingIntrumentation (bool bSkipped, const char *pszLocalNodeId, IHMC_VOI::Rank *pRank);
            MatchmakingIntrumentation (bool bSkipped, const char *pszLocalNodeId, float fThreshold, IHMC_VOI::Rank *pRank);
            virtual ~MatchmakingIntrumentation (void);

            void setNextHopNodeId (const char *pszNextHopNodeID);
            const char * getType (void);

            bool operator == (const MatchmakingIntrumentation &rhsMatchmakingInstrumentation);

            static const char *PUSH_TYPE;
            static const char *PULL_TYPE;

        private:
            friend class Instrumentator;

            const bool _bSkipped;
            const float _fThreshold;
            IHMC_VOI::Rank *_pRank;
            const NOMADSUtil::String _localNodeId;
            NOMADSUtil::String _nextHopNodeId;
    };

    typedef NOMADSUtil::PtrLList<MatchmakingIntrumentation> Instrumentations;
}

#endif    /* INCL_MATCHMAKING_INTRUMENTATION_H */
