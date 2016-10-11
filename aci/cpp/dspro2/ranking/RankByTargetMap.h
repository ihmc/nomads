/*
 * RankByTargetMap.h
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on March 2, 2016, 2:26 PM
 */

#ifndef INCL_RANK_BY_TARGET_MAP_H
#define	INCL_RANK_BY_TARGET_MAP_H

#include "StringStringHashtable.h"

namespace IHMC_ACI
{
    class RankByTargetMap
    {
        public:
            RankByTargetMap (void);
            RankByTargetMap (const char *pszNodeId, float fRank);
            RankByTargetMap (RankByTargetMap &rhsRankByTargetMap);
            ~RankByTargetMap (void);

            void add (const char *pszNodeId, float fRank);
            float get (const char *pszNodeId);
            bool contains (const char *pszNodeId) const;
            bool isEmpty (void) const;
            void remove (const char *pszNodeId);
            NOMADSUtil::String toString (void);

            RankByTargetMap & operator += (RankByTargetMap &rhsNodeIdSet);
            RankByTargetMap & operator = (RankByTargetMap &rhsNodeIdSet);

        private:
            NOMADSUtil::StringStringHashtable _rankByTarget;
    };
}

#endif    /* INCL_RANK_BY_TARGET_MAP_H */

