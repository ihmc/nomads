/*
 * Score.cpp
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

#include "Score.h"

#include "Rank.h"

using namespace IHMC_VOI;

Score::Score (Novelty n, Rank *pR)
    : novelty (n),
      pRank (pR)
{
}

Score::~Score (void)
{
    if (pRank != NULL) {
        delete pRank;
    }
}

bool Score::operator > (const Score &rhsScore) const
{
    return (*pRank > *rhsScore.pRank);
}

bool Score::operator < (const Score &rhsScore) const
{
    return (*pRank < *rhsScore.pRank);
}

const char * IHMC_VOI::toString (Score::Novelty n)
{
    switch (n)
    {
        case Score::CRITICAL: return "CRITICAL";
        case Score::SIGNIFICANT: return "SIGNIFICANT";
        case Score::INSIGNIFICANT: return "INSIGNIFICANT";
        default: return "UNKNOWN";
    }
}
