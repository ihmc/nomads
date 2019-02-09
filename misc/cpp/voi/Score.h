/*
 * Score.h
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
 * Created on March 1, 2017, 3:59 PM
 */

#ifndef INCL_SCORE_H
#define INCL_SCORE_H

#include  "PtrLList.h"

namespace IHMC_VOI
{
    struct Rank;

    struct Score
    {
        enum Novelty
        {
            CRITICAL = 0x00,            // It should increase the relevance (rank)
            SIGNIFICANT = 0x01,         // It does not change the relevance (rank)
            INSIGNIFICANT = 0x02        // It should decrease the relevance (rank)
        };

        Score (Novelty n = CRITICAL, Rank *pR = NULL);
        ~Score (void);

        bool operator > (const Score &rhsScore) const;
        bool operator < (const Score &rhsScore) const;

        Novelty novelty;
        Rank *pRank;
    };

    const char * toString (Score::Novelty n);

    typedef NOMADSUtil::PtrLList<Score> ScoreList;
}

#endif	/* INCL_SCORE_H */

